#include <aegis.hpp>
#include "bot.h"
#include "includes.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <getopt.h>
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "database.h"
#include "config.h"
#include "regex.h"
#include "stringops.h"
#include "rss.h"
#include "modules.h"

Bot::Bot(bool development, aegis::core &aegiscore) : dev(development), thr_userqueue(nullptr), thr_presence(nullptr), terminate(false), core(aegiscore), sent_messages(0), received_messages(0) {

	Loader = new ModuleLoader(this);

	Loader->LoadAll();

	thr_userqueue = new std::thread(&Bot::SaveCachedUsersThread, this);
	thr_presence = new std::thread(&Bot::UpdatePresenceThread, this);
}

void Bot::DisposeThread(std::thread* t) {
	if (t) {
		t->join();
		delete t;
	}

}

Bot::~Bot() {
	terminate = true;

	DisposeThread(thr_userqueue);
	DisposeThread(thr_presence);

	delete Loader;
}

std::string Bot::GetConfig(const std::string &name) {
	json document;
	std::ifstream configfile("../config.json");
	configfile >> document;
	return document[name].get<std::string>();
}

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

	do {
		for (auto i = gc.guild.channels.begin(); i != gc.guild.channels.end(); ++i) {
			getSettings(this, i->id.get(), gc.guild.id.get());
		}
	} while (false);

	for (auto i = gc.guild.members.begin(); i != gc.guild.members.end(); ++i) {
		do {
			std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
			userqueue.push(i->_user);
		} while (false);
	}

	FOREACH_MOD(I_OnGuildCreate, OnGuildCreate(gc));
}

void Bot::SaveCachedUsersThread() {
	time_t last_message = time(NULL);
	aegis::gateway::objects::user u;
	while (!this->terminate) {
		if (!userqueue.empty()) {
			do {
				std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
				u = userqueue.front();
				userqueue.pop();
			} while (false);
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

void Bot::UpdatePresenceThread() {
	std::this_thread::sleep_for(std::chrono::seconds(120));
	uint64_t minutes = 0;
	while (!this->terminate) {
		int64_t servers = core.get_guild_count();
		int64_t users = core.get_member_count();
		int64_t channel_count = core.channels.size();
		int64_t ram = GetRSS();

		db::resultset rs_fact = db::query("SELECT count(key_word) AS total FROM infobot", std::vector<std::string>());
		core.update_presence(Comma(from_string<size_t>(rs_fact[0]["total"], std::dec)) + " facts, on " + Comma(servers) + " servers with " + Comma(users) + " users across " + Comma(core.shard_max_count) + " shards", aegis::gateway::objects::activity::Watching);
		db::query("INSERT INTO infobot_discord_counts (shard_id, dev, user_count, server_count, shard_count, channel_count, sent_messages, received_messages, memory_usage) VALUES('?','?','?','?','?','?','?','?','?') ON DUPLICATE KEY UPDATE user_count = '?', server_count = '?', shard_count = '?', channel_count = '?', sent_messages = '?', received_messages = '?', memory_usage = '?'",
				{std::to_string(0), std::to_string((uint32_t)dev), std::to_string(users), std::to_string(servers), std::to_string(core.shard_max_count),
				std::to_string(channel_count), std::to_string(sent_messages), std::to_string(received_messages), std::to_string(ram),
				std::to_string(users), std::to_string(servers), std::to_string(core.shard_max_count),
				std::to_string(channel_count), std::to_string(sent_messages), std::to_string(received_messages), std::to_string(ram)
				});
		if (++minutes > 10) {
			minutes = sent_messages = received_messages = 0;
		}

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

		db::resultset rs_votes = db::query("SELECT id, snowflake_id, UNIX_TIMESTAMP(vote_time) AS vote_time, origin, rolegiven FROM infobot_votes", {});
		aegis::guild* home = core.find_guild(from_string<int64_t>(Bot::GetConfig("home"), std::dec));
		if (home) {
			/* Process removals first */
			for (auto vote = rs_votes.begin(); vote != rs_votes.end(); ++vote) {
				int64_t member_id = from_string<int64_t>((*vote)["snowflake_id"], std::dec);
				aegis::user* user = core.find_user(member_id);
				if (user) {
					/* FIXME: If they've voted again, we shouldnt take their role away below. check for this. */
					if ((*vote)["rolegiven"] == "1") {
						/* Role was already given, take away the role and remove the vote IF the date is too far in the past.
						 * Votes last 24 hours.
						 */
						uint64_t role_timestamp = from_string<uint64_t>((*vote)["vote_time"], std::dec);
						if (time(NULL) - role_timestamp > 86400) {
							db::query("DELETE FROM infobot_votes WHERE id = ?", {(*vote)["id"]});
							home->remove_guild_member_role(member_id, from_string<int64_t>(Bot::GetConfig("vote_role"), std::dec));
							core.log->info("Removing vanity role from {}", member_id);
						}
					}
				}
			}
			/* Now additions, so that if they've re-voted, it doesnt remove it */
			for (auto vote = rs_votes.begin(); vote != rs_votes.end(); ++vote) {
				int64_t member_id = from_string<int64_t>((*vote)["snowflake_id"], std::dec);
				aegis::user* user = core.find_user(member_id);
				if (user) {
					/* FIXME: If they've voted again, we shouldnt take their role away below. check for this. */
					if ((*vote)["rolegiven"] == "0") {
						/* Role not yet given, give the role and set rolegiven to 1 */
						core.log->info("Adding vanity role to {}", member_id);
						home->add_guild_member_role(member_id, from_string<int64_t>(Bot::GetConfig("vote_role"), std::dec));
						db::query("UPDATE infobot_votes SET rolegiven = 1 WHERE snowflake_id = ?", {(*vote)["snowflake_id"]});
					}
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::seconds(30));
	}
}

void Bot::onMember(aegis::gateway::events::guild_member_add gma) {
	std::string userid = std::to_string(gma.member._user.id.get());
	std::string bot = gma.member._user.is_bot() ? "1" : "0";
	db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {userid, gma.member._user.username, gma.member._user.discriminator, gma.member._user.avatar, bot, gma.member._user.username, gma.member._user.discriminator, gma.member._user.avatar});
}

int64_t Bot::getID() {
	return this->user.id.get();
}

void Bot::onReady(aegis::gateway::events::ready ready) {
	this->user = ready.user;
	core.log->info("Ready! Online as {}#{} ({})", this->user.username, this->user.discriminator, this->getID());
}

void Bot::onMessage(aegis::gateway::events::message_create message) {

	json settings;
	do {
		std::lock_guard<std::mutex> input_lock(channel_hash_mutex);
		settings = getSettings(this, message.msg.get_channel_id().get(), message.msg.get_guild_id().get());
	} while (false);

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

void Bot::onChannel(aegis::gateway::events::channel_create channel_create) {
	getSettings(this, channel_create.channel.id.get(), channel_create.channel.guild_id.get());
}

void Bot::onChannelDelete(aegis::gateway::events::channel_delete cd) {
	db::query("DELETE FROM infobot_discord_settings WHERE id = '?'", {std::to_string(cd.channel.id.get())});
}

void Bot::onServerDelete(aegis::gateway::events::guild_delete gd) {
	db::query("DELETE FROM infobot_discord_settings WHERE guild_id = '?'", {std::to_string(gd.guild_id.get())});
	db::query("DELETE FROM infobot_shard_map WHERE guild_id = '?'", {std::to_string(gd.guild_id.get())});
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

