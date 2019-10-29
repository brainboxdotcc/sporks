#include "sleepy_discord/sleepy_discord.h"
#include "bot.h"
#include "includes.h"
#include "readline.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <queue>
#include "database.h"
#include "config.h"
#include "regex.h"
#include "stringops.h"
#include "help.h"

QueueStats Bot::GetQueueStats() {
	QueueStats q;

	q.inputs = inputs.size();
	q.outputs = outputs.size();
	q.users = userqueue.size();

	return q;
}

Bot::Bot(const std::string &token, uint32_t shard_id, uint32_t max_shards, bool development) : SleepyDiscord::DiscordClient(token, SleepyDiscord::USER_CONTROLED_THREADS), dev(development), thr_input(nullptr), thr_output(nullptr), thr_userqueue(nullptr), thr_presence(nullptr), terminate(false), ShardID(shard_id), MaxShards(max_shards) {

	this->setShardID(ShardID, MaxShards);
	this->DisablePresenceUpdates();

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

void Bot::onServer(const SleepyDiscord::Server &server) {
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
}

void Bot::SaveCachedUsersThread() {
	SleepyDiscord::User u;
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
	}
}

void Bot::UpdatePresenceThread() {
	std::this_thread::sleep_for(std::chrono::seconds(30));
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
	}
}

void Bot::onMember(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, SleepyDiscord::ServerMember member) {
	std::string userid = member.user.ID;
	std::string bot = member.user.bot ? "1" : "0";
	db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {userid, member.user.username, member.user.discriminator, member.user.avatar, bot, member.user.username, member.user.discriminator, member.user.avatar});
	this->userList[userid] = member.user;
}

void Bot::onReady(const SleepyDiscord::Ready &ready) {
	this->user = ready.user;
	std::cout << "Ready! Online as " << this->user.username <<"#" << this->user.discriminator << " (" << std::string(this->getID()) << ")\n";
}

void Bot::onMessage(const SleepyDiscord::Message &message) {

	rapidjson::Document settings;
	do {
		std::lock_guard<std::mutex> input_lock(channel_hash_mutex);
		settings = getSettings(this, message.channelID, message.serverID);
	} while (false);

	/* Ignore self, and bots */
	if (message.author.ID != this->getID() && message.author.bot == false) {

		/* Ignore anyone on ignore list */
		std::vector<uint64_t> ignorelist = settings::GetIgnoreList(settings);
		if (std::find(ignorelist.begin(), ignorelist.end(), from_string<uint64_t>(message.author.ID, std::dec)) != ignorelist.end()) {
			std::cout << "Message " << std::string(message.ID) << " dropped, user on channel ignore list" << std::endl;
			return;
		}

		/* Replace all mentions with raw nicknames */
		bool mentioned = false;
		std::string mentions_removed = message.content;
		for (auto m = message.mentions.begin(); m != message.mentions.end(); ++m) {
			mentions_removed = ReplaceString(mentions_removed, std::string("<@") + std::string(m->ID) + ">", m->username);
			/* Note: I know there's a message::isMentioned(), but we're looping here anyway so might as well roll this into one */
			if (m->ID == this->getID()) {
				mentioned = true;
			}
		}

		std::cout << "<" << message.author.username << "> " << mentions_removed << std::endl;

		std::string botusername = this->user.username;

		/* Remove bot's nickname from start of message, if it's there */
		while (mentions_removed.substr(0, botusername.length()) == botusername) {
			mentions_removed = trim(mentions_removed.substr(botusername.length(), mentions_removed.length()));
		}

		std::vector<std::string> param;
		if (mentioned && helpmessage->Match(mentions_removed, param)) {
			std::string section = "basic";
			if (param.size() > 2) {
				section = param[2];
			}
			GetHelp(this, section, message.channelID, botusername, std::string(this->getID()), message.author.username, message.author.ID);
		} else if (mentioned && configmessage->Match(trim(mentions_removed), param)) {
			/* Config command */
			DoConfig(this, param, message.channelID, message);
		} else {
			QueueItem query;
			query.message = mentions_removed;
			query.channelID = message.channelID;
			query.serverID = message.serverID;
			query.username = message.author.username;
			query.mentioned = mentioned;
			do {
				std::lock_guard<std::mutex> input_lock(input_mutex);
				inputs.push(query);
			} while (false);
		}
	}
}

void Bot::onChannel(const SleepyDiscord::Channel &channel) {
	do {
		std::lock_guard<std::mutex> hash_lock(channel_hash_mutex);
		channelList[std::string(channel.ID)] = channel;
	} while (false);
	getSettings(this, channel, channel.serverID);
}


int main(int argc, char** argv) {

	bool dev = (argc >= 2 && strcmp(argv[1], "--dev") == 0);
	std::string token = (dev ? Bot::GetConfig("devtoken") : Bot::GetConfig("livetoken"));

	if (!db::connect(Bot::GetConfig("dbhost"), Bot::GetConfig("dbuser"), Bot::GetConfig("dbpass"), Bot::GetConfig("dbname"), from_string<uint32_t>(Bot::GetConfig("dbport"), std::dec))) {
		std::cerr << "Database connection failed\n";
		exit(2);
	}

	while (true) {
		Bot client(token, 0, 0, dev);
	
		try {
			client.run();
		}
		catch (SleepyDiscord::ErrorCode e) {
			std::cout << "Oof! #" << e << std::endl;
		}

		::sleep(5);
	}
}

