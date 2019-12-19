#include <aegis.hpp>
#include <sporks/bot.h>
#include <sporks/includes.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sporks/database.h>
#include <sporks/config.h>
#include <sporks/regex.h>
#include <sporks/stringops.h>
#include <sporks/modules.h>

/**
 * Constructor (creates threads, loads all modules)
 */
Bot::Bot(bool development, aegis::core &aegiscore) : dev(development), thr_userqueue(nullptr), thr_presence(nullptr), terminate(false), core(aegiscore), sent_messages(0), received_messages(0) {

	Loader = new ModuleLoader(this);

	Loader->LoadAll();

	thr_userqueue = new std::thread(&Bot::SaveCachedUsersThread, this);
	thr_presence = new std::thread(&Bot::UpdatePresenceThread, this);
}

/**
 * Join and delete a thread
 */
void Bot::DisposeThread(std::thread* t) {
	if (t) {
		t->join();
		delete t;
	}

}

/**
 * Destructor
 */
Bot::~Bot() {
	terminate = true;

	DisposeThread(thr_userqueue);
	DisposeThread(thr_presence);

	delete Loader;
}

/**
 * Returns the named string value from config.json
 */
std::string Bot::GetConfig(const std::string &name) {
	json document;
	std::ifstream configfile("../config.json");
	configfile >> document;
	return document[name].get<std::string>();
}

/**
 * Returns true if the bot is running in development mode (different token)
 */
bool Bot::IsDevMode() {
	return dev;
}

/**
 * On adding a new server, the details of that server are inserted or updated in the shard map. We also make sure settings
 * exist for each channel on the server by calling getSettings() for each channel and throwing the result away, which causes
 * record creation. New users for the guild are pushed into the userqueue which is processed in a separate thread within
 * SaveCachedUsersThread().
 */
void Bot::onServer(aegis::gateway::events::guild_create gc) {

	core.log->info("Adding server #{}: {}", gc.guild.id.get(), gc.guild.name);

	db::query("INSERT INTO infobot_shard_map (guild_id, shard_id, name, icon, unavailable) VALUES('?','?','?','?','?') ON DUPLICATE KEY UPDATE shard_id = '?', name = '?', icon = '?', unavailable = '?'", 
		{ 
			std::to_string(gc.guild.id.get()),
			std::to_string(gc.shard.get_id()),
			gc.guild.name,
			gc.guild.icon,
			std::to_string(gc.guild.unavailable),
			std::to_string(gc.shard.get_id()),
			gc.guild.name,
			gc.guild.icon,
			std::to_string(gc.guild.unavailable)
		}
	);

	for (auto i = gc.guild.channels.begin(); i != gc.guild.channels.end(); ++i) {
		getSettings(this, i->id.get(), gc.guild.id.get());
	}

	for (auto i = gc.guild.members.begin(); i != gc.guild.members.end(); ++i) {
		std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
		userqueue.push(i->_user);
	}

	FOREACH_MOD(I_OnGuildCreate, OnGuildCreate(gc));
}

/**
 * This function runs in its own thread, which commits new users to the database (for dashboard use)
 * when there are entries in the queue. We don't do this on guild creation, because this would slow
 * down the bot too much.
 */
void Bot::SaveCachedUsersThread() {
	time_t last_message = time(NULL);
	aegis::gateway::objects::user u;
	while (!this->terminate) {
		if (!userqueue.empty()) {
			{
				std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
				u = userqueue.front();
				userqueue.pop();
			};
			std::string userid = std::to_string(u.id.get());
			std::string bot = u.is_bot() ? "1" : "0";
			db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {userid, u.username, u.discriminator, u.avatar, bot, u.username, u.discriminator, u.avatar});
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		} else {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		if (time(NULL) > last_message) {
			if (userqueue.size() > 0) {
				core.log->info("User queue size: {} objects", userqueue.size());
			}
			last_message = time(NULL) + 60;
		}
	}
}

/**
 * This runs its own thread that wakes up every 30 seconds (after an initial 2 minute warmup).
 * Modules can attach to it for a simple 30 second interval timer via the OnPresenceUpdate() method.
 * The code here simply updates the stats on the shards in the database.
 */
void Bot::UpdatePresenceThread() {
	std::this_thread::sleep_for(std::chrono::seconds(120));

	while (!this->terminate) {

		const aegis::shards::shard_mgr& s = core.get_shard_mgr();
		const std::vector<std::unique_ptr<aegis::shards::shard>>& shards = s.get_shards();
		for (auto i = shards.begin(); i != shards.end(); ++i) {
			const aegis::shards::shard* shard = i->get();
			db::query("INSERT INTO infobot_shard_status (id, connected, online, uptime, transfer, transfer_compressed) VALUES('?','?','?','?','?','?') ON DUPLICATE KEY UPDATE connected = '?', online = '?', uptime = '?', transfer = '?', transfer_compressed = '?'", 
				{
					std::to_string(shard->get_id()),
					std::to_string(shard->is_connected()),
					std::to_string(shard->is_online()),
					std::to_string(shard->uptime()),
					std::to_string(shard->get_transfer_u()),
					std::to_string(shard->get_transfer()),
					std::to_string(shard->is_connected()),
					std::to_string(shard->is_online()),
					std::to_string(shard->uptime()),
					std::to_string(shard->get_transfer_u()),
					std::to_string(shard->get_transfer())
				}
			);
		}

		FOREACH_MOD(I_OnPresenceUpdate, OnPresenceUpdate());

		std::this_thread::sleep_for(std::chrono::seconds(30));
	}
}

/**
 * Stores a new guild member to the database for use in the dashboard
 */
void Bot::onMember(aegis::gateway::events::guild_member_add gma) {
	std::string userid = std::to_string(gma.member._user.id.get());
	std::string bot = gma.member._user.is_bot() ? "1" : "0";
	db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {userid, gma.member._user.username, gma.member._user.discriminator, gma.member._user.avatar, bot, gma.member._user.username, gma.member._user.discriminator, gma.member._user.avatar});
	FOREACH_MOD(I_OnGuildMemberAdd, OnGuildMemberAdd(gma));
}

/**
 * Returns the bot's snowflake id
 */
int64_t Bot::getID() {
	return this->user.id.get();
}

/**
 * Announces that the bot is online. Each shard receives one of the events.
 */
void Bot::onReady(aegis::gateway::events::ready ready) {
	this->user = ready.user;
	core.log->info("Ready! Online as {}#{} ({})", this->user.username, this->user.discriminator, this->getID());
	FOREACH_MOD(I_OnReady, OnReady(ready));
}

/**
 * Called on receipt of each message. We do our own cleanup of the message, sanitising any
 * mentions etc from the text before passing it along to modules. The bot's builtin ignore list
 * and a hard coded check against bots/webhooks and itself happen before any module calls,
 * and can't be overridden.
 */
void Bot::onMessage(aegis::gateway::events::message_create message) {

	json settings;
	{
		std::lock_guard<std::mutex> input_lock(channel_hash_mutex);
		settings = getSettings(this, message.msg.get_channel_id().get(), message.msg.get_guild_id().get());
	};

	/* Ignore self, and bots */
	if (message.msg.get_user().get_id() != user.id && message.msg.get_user().is_bot() == false) {

		received_messages++;

		/* Ignore anyone on ignore list */
		std::vector<uint64_t> ignorelist = settings::GetIgnoreList(settings);
		if (std::find(ignorelist.begin(), ignorelist.end(), message.msg.get_user().get_id().get()) != ignorelist.end()) {
			core.log->info("Message #{} dropped, user on channel ignore list", message.msg.get_id().get());
			return;
		}

		/* Replace all mentions with raw nicknames */
		bool mentioned = false;
		std::string mentions_removed = message.msg.get_content();
		std::vector<std::string> stringmentions;
		for (auto m = message.msg.mentions.begin(); m != message.msg.mentions.end(); ++m) {
			stringmentions.push_back(std::to_string(m->get()));
			mentions_removed = ReplaceString(mentions_removed, std::string("<@") + std::to_string(m->get()) + ">", core.find_user(*m)->get_username());
			mentions_removed = ReplaceString(mentions_removed, std::string("<@!") + std::to_string(m->get()) + ">", core.find_user(*m)->get_username());
			if (*m == user.id) {
				mentioned = true;
			}
		}

		core.log->info("<{}> {}", message.msg.get_user().get_username(), mentions_removed);

		std::string botusername = this->user.username;

		/* Remove bot's nickname from start of message, if it's there */
		while (mentions_removed.substr(0, botusername.length()) == botusername) {
			mentions_removed = trim(mentions_removed.substr(botusername.length(), mentions_removed.length()));
		}
		/* Remove linefeeds, they mess with botnix */
		mentions_removed = trim(ReplaceString(mentions_removed, "\r\n", " "));

		/* Call modules */
		FOREACH_MOD(I_OnMessage,OnMessage(message, mentions_removed, mentioned, stringmentions));

		core.log->flush();
	}
}

/**
 * This reates a new settings entry in the database for a channel whenever a new channel is created
 */
void Bot::onChannel(aegis::gateway::events::channel_create channel_create) {
	getSettings(this, channel_create.channel.id.get(), channel_create.channel.guild_id.get());
	FOREACH_MOD(I_OnChannelCreate, OnChannelCreate(channel_create));
}

/**
 * Removes settings entries from the database as channels that refer to them are removed
 */
void Bot::onChannelDelete(aegis::gateway::events::channel_delete cd) {
	db::query("DELETE FROM infobot_discord_settings WHERE id = '?'", {std::to_string(cd.channel.id.get())});
	FOREACH_MOD(I_OnChannelDelete, OnChannelDelete(cd));
}

/**
 * Removes settings for all channels on a server if the bot is kicked from a server,
 * also removes the entry with server details from the shard map.
 */
void Bot::onServerDelete(aegis::gateway::events::guild_delete gd) {
	db::query("DELETE FROM infobot_discord_settings WHERE guild_id = '?'", {std::to_string(gd.guild_id.get())});
	db::query("DELETE FROM infobot_shard_map WHERE guild_id = '?'", {std::to_string(gd.guild_id.get())});
	FOREACH_MOD(I_OnGuildDelete, OnGuildDelete(gd));
}

int main(int argc, char** argv) {

	int dev = 0;	/* Note: getopt expects ints, this is actually treated as bool */

	/* Parse command line parameters using getopt() */
	struct option longopts[] =
	{
		{ "dev",	   no_argument,		&dev,	1  },
		{ 0, 0, 0, 0 }
	};

	/* Yes, getopt is ugly, but what you gonna do... */
	int index;
	char arg;
	while ((arg = getopt_long_only(argc, argv, "", longopts, &index)) != -1) {
		switch (arg) {
			case 0:
				/* getopt_long_only() set an int variable, just keep going */
			break;
			case '?':
			default:
				std::cerr << "Unknown parameter '" << argv[optind - 1] << "'" << std::endl;
				std::cerr << "Usage: %s [--dev] [--shardid <n>] [--numshards <n>]" << std::endl;
				exit(1);
			break;
		}
	}


	/* Get the correct token from config file for either development or production environment */
	std::string token = (dev ? Bot::GetConfig("devtoken") : Bot::GetConfig("livetoken"));

	/* Connect to SQL database */
	if (!db::connect(Bot::GetConfig("dbhost"), Bot::GetConfig("dbuser"), Bot::GetConfig("dbpass"), Bot::GetConfig("dbname"), from_string<uint32_t>(Bot::GetConfig("dbport"), std::dec))) {
		std::cerr << "Database connection failed\n";
		exit(2);
	}

	/* It's go time! */
	while (true) {

		/* Aegis core routes websocket events and does all the API magic */
		aegis::core aegis_bot(aegis::create_bot_t().file_logging(true).log_level(spdlog::level::trace).token(token).force_shard_count(dev ? 1 : 10));
		aegis_bot.wsdbg = false;

		/* Bot class handles application logic */
		Bot client(dev, aegis_bot);

		/* Attach events to the Bot class methods from aegis::core */
		aegis_bot.set_on_message_create(std::bind(&Bot::onMessage, &client, std::placeholders::_1));
		aegis_bot.set_on_ready(std::bind(&Bot::onReady, &client, std::placeholders::_1));
		aegis_bot.set_on_channel_create(std::bind(&Bot::onChannel, &client, std::placeholders::_1));
		aegis_bot.set_on_guild_member_add(std::bind(&Bot::onMember, &client, std::placeholders::_1));
		aegis_bot.set_on_guild_create(std::bind(&Bot::onServer, &client, std::placeholders::_1));
		aegis_bot.set_on_guild_delete(std::bind(&Bot::onServerDelete, &client, std::placeholders::_1));
		aegis_bot.set_on_channel_delete(std::bind(&Bot::onChannelDelete, &client, std::placeholders::_1));
	
		try {
			/* Actually connect and start the event loop */
			aegis_bot.run();
			aegis_bot.yield();
		}
		catch (std::exception e) {
			aegis_bot.log->error("Oof! {}", e.what());
		}

		/* Reconnection delay to prevent hammering discord */
		::sleep(30);
	}
}

