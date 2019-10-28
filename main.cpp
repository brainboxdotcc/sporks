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
#include "queue.h"
#include "infobot.h"
#include "response.h"
#include "config.h"
#include "regex.h"
#include "stringops.h"
#include "help.h"

Queue inputs;
Queue outputs;

std::mutex input_mutex;
std::mutex output_mutex;
std::mutex channel_hash_mutex;

std::queue<SleepyDiscord::User> userqueue;
std::mutex user_cache_mutex;

rapidjson::Document config;

QueueStats Bot::GetQueueStats() {
	QueueStats q;

	q.inputs = inputs.size();
	q.outputs = outputs.size();
	q.users = userqueue.size();

	return q;
}

void Bot::setup() {
	this->setShardID(0, 0);
	helpmessage = new PCRE("^help(|\\s+(.+?))$", true);
	configmessage = new PCRE("^config(|\\s+(.+?))$", true);
}

void Bot::onServer(SleepyDiscord::Server server) {
	serverList.push_back(server);
	std::string serverID = server.ID;
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
		this->userList[std::string(i->ID)] = *i;
		this->nickList[serverID].push_back(i->user.username);
	}
}

void SaveCachedUsers() {
	SleepyDiscord::User u;
	time_t last_message = time(NULL);
	while (true) {
		if (!userqueue.empty()) {
			do {
				std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
				u = userqueue.front();
				userqueue.pop();
			} while (false);
			std::string userid = u.ID;
			std::string bot = u.bot ? "1" : "0";
			db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {userid, u.username, u.discriminator, u.avatar, bot, u.username, u.discriminator, u.avatar});
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (time(NULL) > last_message) {
			if (userqueue.size() > 0) {
				std::cout << "User queue size: " << userqueue.size() << std::endl;
			}
			last_message = time(NULL) + 60;
		}
	}
}

void UpdatePresence(Bot* bot) {
	std::this_thread::sleep_for(std::chrono::seconds(30));
	while (true) {
		size_t servers = bot->serverList.size();
		size_t users = 0;
		for (size_t i = 0; i < bot->serverList.size(); ++i) {
			users += bot->serverList[i].members.size();
		}
		db::resultset rs_fact = db::query("SELECT count(key_word) AS total FROM infobot", std::vector<std::string>());
		bot->updateStatus(Comma(from_string<size_t>(rs_fact[0]["total"], std::dec)) + " facts on " + Comma(servers) + " servers, " + Comma(users) + " total users", 0, SleepyDiscord::Status::online, false, 3);
		std::this_thread::sleep_for(std::chrono::seconds(120));
	}
}

void Bot::onMember(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, SleepyDiscord::ServerMember member) {
	std::string userid = member.user.ID;
	std::string bot = member.user.bot ? "1" : "0";
	db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {userid, member.user.username, member.user.discriminator, member.user.avatar, bot, member.user.username, member.user.discriminator, member.user.avatar});
	this->userList[userid] = member.user;
}

void Bot::onReady(SleepyDiscord::Ready ready) {
	this->user = ready.user;
	std::cout << "Ready! Online as " << this->user.username <<"#" << this->user.discriminator << " (" << std::string(this->getID()) << ")\n";
}

void Bot::onMessage(SleepyDiscord::Message message) {

	rapidjson::Document settings;
	do {
		std::lock_guard<std::mutex> input_lock(channel_hash_mutex);
		settings = getSettings(this, message.channelID, message.serverID);
	} while (false);

	if (message.author.ID != this->getID()) {

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
			DoConfig(this, param, message.channelID, message.author);
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

void Bot::onChannel(SleepyDiscord::Channel channel) {
	do {
		std::lock_guard<std::mutex> hash_lock(channel_hash_mutex);
		channelList[std::string(channel.ID)] = channel;
	} while (false);
	getSettings(this, channel, channel.serverID);
}


int main(int argc, char** argv) {
	std::ifstream configfile("../config.json");
	rapidjson::IStreamWrapper wrapper(configfile);
	config.ParseStream(wrapper);
	if (!config.IsObject()) {
		std::cerr << "../config.json not found, or doesn't contain an object" << std::endl;
		exit(1);
	}
	std::string token = (argc >= 2 && strcmp(argv[1], "--dev") == 0) ? config["devtoken"].GetString() : config["livetoken"].GetString();

	if (!db::connect(config["dbhost"].GetString(), config["dbuser"].GetString(), config["dbpass"].GetString(), config["dbname"].GetString(), config["dbport"].GetInt())) {
		std::cerr << "Database connection failed\n";
		exit(2);
	}

	while (true) {
		Bot client(token, SleepyDiscord::USER_CONTROLED_THREADS);
		client.setup();
		std::thread infobot(infobot_socket, &client, &input_mutex, &output_mutex, &channel_hash_mutex, &inputs, &outputs, &config);
		std::thread responses(send_responses, &client, &output_mutex, &channel_hash_mutex, &outputs);
		std::thread userqueueupdate(SaveCachedUsers);
		std::thread presenceupdate(UpdatePresence, &client);
	
		try {
			client.run();
		}
		catch (SleepyDiscord::ErrorCode e) {
			std::cout << "Oof! #" << e << std::endl;
		}

		::sleep(5);
	}
}

