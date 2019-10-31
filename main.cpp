#include <aegis.hpp>
#include "bot.h"
#include "includes.h"
#include "readline.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <getopt.h>
#include "database.h"
#include "config.h"
#include "regex.h"
#include "stringops.h"
#include "help.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

QueueStats Bot::GetQueueStats() {
	QueueStats q;

	/*q.inputs = inputs.size();
	q.outputs = outputs.size();
	q.users = userqueue.size();*/

	return q;
}

Bot::Bot(uint32_t shard_id, uint32_t max_shards, bool development, aegis::core &aegiscore) : dev(development), thr_input(nullptr), thr_output(nullptr), thr_userqueue(nullptr), thr_presence(nullptr), terminate(false), core(aegiscore), ShardID(shard_id), MaxShards(max_shards) {

	helpmessage = new PCRE("^help(|\\s+(.+?))$", true);
	configmessage = new PCRE("^config(|\\s+(.+?))$", true);

	thr_input = new std::thread(&Bot::InputThread, this);
	thr_output = new std::thread(&Bot::OutputThread, this);
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
	delete helpmessage;
	delete configmessage;

	terminate = true;

	DisposeThread(thr_input);
	DisposeThread(thr_output);
	DisposeThread(thr_userqueue);
	DisposeThread(thr_presence);
}

std::string Bot::GetConfig(const std::string &name) {
	rapidjson::Document config;
	std::ifstream configfile("../config.json");
	rapidjson::IStreamWrapper wrapper(configfile);
	config.ParseStream(wrapper);
	if (!config.IsObject()) {
		std::cerr << "../config.json not found, or doesn't contain an object" << std::endl;
		exit(1);
	}
	return config[name.c_str()].GetString();
}

/*void Bot::onServer(const SleepyDiscord::Server &server) {
	std::string serverID = server.ID;
	serverList[serverID] = server;
	std::cout << "Adding server #" << std::string(server.ID) << ": " << server.name << "\n";
	do {
		std::lock_guard<std::mutex> hash_lock(channel_hash_mutex);
		for (auto i = server.channels.begin(); i != server.channels.end(); ++i) {
			channelList[std::string(i->ID)] = *i;
			getSettings(this, *i, server.ID);
		}
	} while (false);
	this->nickList[serverID] = std::vector<std::string>();
	for (auto i = server.members.begin(); i != server.members.end(); ++i) {
		do {
			std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
			userqueue.push(i->user);
		} while (false);
		this->userList[std::string(i->ID)] = i->user;
		this->nickList[serverID].push_back(i->user.username);
	}
}*/

void Bot::SaveCachedUsersThread() {
/*	SleepyDiscord::User u;
	time_t last_message = time(NULL);
	while (!this->terminate) {
		if (!userqueue.empty()) {
			do {
				std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
				u = userqueue.front();
				userqueue.pop();
			} while (false);
			std::string userid = u.ID;
			std::string bot = u.bot ? "1" : "0";
			db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {userid, u.username, u.discriminator, u.avatar, bot, u.username, u.discriminator, u.avatar});
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		} else {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		if (time(NULL) > last_message) {
			if (userqueue.size() > 0) {
				std::cout << "User queue size: " << userqueue.size() << std::endl;
			}
			last_message = time(NULL) + 60;
		}
	}*/
}

void Bot::UpdatePresenceThread() {
/*	std::this_thread::sleep_for(std::chrono::seconds(30));
	while (true) {
		size_t servers = serverList.size();
		size_t users = 0;
		for (auto i = serverList.begin(); i != serverList.end(); ++i) {
			users += i->second.members.size();
		}
		db::resultset rs_fact = db::query("SELECT count(key_word) AS total FROM infobot", std::vector<std::string>());
		updateStatus(Comma(from_string<size_t>(rs_fact[0]["total"], std::dec)) + " facts on " + Comma(servers) + " servers, " + Comma(users) + " total users", 0, SleepyDiscord::Status::online, false, 3);
		db::query("INSERT INTO infobot_discord_counts (shard_id, dev, user_count, server_count) VALUES('?','?','?','?') ON DUPLICATE KEY UPDATE user_count = '?', server_count = '?', dev = '?'", {std::to_string(ShardID), std::to_string((uint32_t)dev), std::to_string(users), std::to_string(servers), std::to_string(users), std::to_string(servers), std::to_string((uint32_t)dev)});
		std::this_thread::sleep_for(std::chrono::seconds(120));
	}*/
}

/*void Bot::onMember(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, SleepyDiscord::ServerMember member) {
	std::string userid = member.user.ID;
	std::string bot = member.user.bot ? "1" : "0";
	db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {userid, member.user.username, member.user.discriminator, member.user.avatar, bot, member.user.username, member.user.discriminator, member.user.avatar});
	this->userList[userid] = member.user;
}*/

int64_t Bot::getID() {
	return this->user.id.get();
}

void Bot::onReady(aegis::gateway::events::ready ready) {
	this->user = ready.user;
	std::cout << "Ready! Online as " << this->user.username <<"#" << this->user.discriminator << " (" << this->getID() << ")\n";
}

void Bot::onMessage(aegis::gateway::events::message_create message) {

	rapidjson::Document settings;
	do {
		std::lock_guard<std::mutex> input_lock(channel_hash_mutex);
		settings = getSettings(this, message.msg.get_channel_id().get(), message.msg.get_guild_id().get());
	} while (false);

	/* Ignore self, and bots */
	if (message.get_user().get_id().get() != 0 /* FIXME */ && message.get_user().is_bot() == false) {

		/* Ignore anyone on ignore list */
		std::vector<uint64_t> ignorelist = settings::GetIgnoreList(settings);
		if (std::find(ignorelist.begin(), ignorelist.end(), message.get_user().get_id().get()) != ignorelist.end()) {
			std::cout << "Message " << std::to_string(message.msg.get_id().get()) << " dropped, user on channel ignore list" << std::endl;
			return;
		}

		/* Replace all mentions with raw nicknames */
		bool mentioned = false;
		std::string mentions_removed = message.msg.get_content();
		for (auto m = message.msg.mentions.begin(); m != message.msg.mentions.end(); ++m) {
			mentions_removed = ReplaceString(mentions_removed, std::string("<@") + std::to_string(m->get()) + ">", "XXXXX"/*m->username FIXME */);
			/* Note: I know there's a message::isMentioned(), but we're looping here anyway so might as well roll this into one */
			if (m->get() == 0 /* FIXME */) {
				mentioned = true;
			}
		}

		std::cout << "<" << message.get_user().get_username() << "> " << mentions_removed << std::endl;

		std::string botusername = "XXXXX";// FIXME this->user.username;

		/* Remove bot's nickname from start of message, if it's there */
		while (mentions_removed.substr(0, botusername.length()) == botusername) {
			mentions_removed = trim(mentions_removed.substr(botusername.length(), mentions_removed.length()));
		}
		/* Remove linefeeds, they mess with botnix */
		mentions_removed = trim(ReplaceString(mentions_removed, "\r\n", " "));

		/* Hard coded commands, help/config */
		std::vector<std::string> param;
		if (mentioned && helpmessage->Match(mentions_removed, param)) {
			std::string section = "basic";
			if (param.size() > 2) {
				section = param[2];
			}
			GetHelp(this, section, message.msg.get_channel_id().get(), botusername, 0 /* bot id, FIXME */, message.get_user().get_username(), message.get_user().get_id().get());
		} else if (mentioned && configmessage->Match(trim(mentions_removed), param)) {
			/* Config command */
			DoConfig(this, param, message.msg.get_channel_id().get(), message.msg);
		} else {
			/* Everything else goes to the input queue to be processed by botnix */
			QueueItem query;
			query.message = mentions_removed;
			query.channelID = message.channel.get_id().get();
			query.serverID = message.msg.get_guild_id().get();
			query.username = message.get_user().get_username();
			query.mentioned = mentioned;
			do {
				std::lock_guard<std::mutex> input_lock(input_mutex);
				inputs.push(query);
			} while (false);
		}
	}
}

/*void Bot::onChannel(const SleepyDiscord::Channel &channel) {

	do {
		std::lock_guard<std::mutex> hash_lock(channel_hash_mutex);
		channelList[std::string(channel.ID)] = channel;
	} while (false);

	getSettings(this, channel, channel.serverID);
}*/


int main(int argc, char** argv) {

	int dev = 0;	/* Note: getopt expects ints, this is actually treated as bool */
	uint32_t shard_id = 0;
	uint32_t max_shards = 0;

	/* Parse command line parameters using getopt() */
	struct option longopts[] =
	{
		{ "dev",       no_argument,        &dev,    1  },
		{ "shardid",   required_argument,  NULL,   'i' },
		{ "numshards", required_argument,  NULL,   'n' },
		{ 0, 0, 0, 0 }
	};

	/* Yes, getopt is ugly, but what you gonna do... */
	int index;
	char arg;
	while ((arg = getopt_long_only(argc, argv, ":i:n", longopts, &index)) != -1) {
		switch (arg) {
			case 'i':
				/* Shard ID was set */
				shard_id = atoi(optarg);
			break;
			case 'n':
				/* Number of shards was set */
				max_shards = atoi(optarg);
			break;
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

		aegis::core aegis_bot(aegis::create_bot_t().log_level(spdlog::level::trace).token(token));
		aegis_bot.wsdbg = true;

		Bot client(shard_id, max_shards, dev, aegis_bot);

		aegis_bot.set_on_message_create(std::bind(&Bot::onMessage, &client, std::placeholders::_1));
	
		try {
			/* Actually connect and start the event loop */
			aegis_bot.run();
			aegis_bot.yield();
		}
		catch (std::exception e) {
			std::cout << "Oof!" << std::endl;
		}

		/* Reconnection delay to prevent hammering discord */
		::sleep(30);
	}
}

