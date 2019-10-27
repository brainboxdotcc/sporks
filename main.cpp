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

std::queue<SleepyDiscord::User> usercache;
std::mutex user_cache_mutex;

rapidjson::Document config;

void Bot::setup() {
	this->setShardID(0, 0);
	helpmessage = new PCRE("^help(|\\s+(.+?))$", true);
	configmessage = new PCRE("^config(|\\s+(.+?))$", true);
}

void Bot::onServer(SleepyDiscord::Server server) {
	serverList.push_back(server);
	std::cout << "Adding server #" << std::string(server.ID) << ": " << server.name << "\n";
	do {
		std::lock_guard<std::mutex> hash_lock(channel_hash_mutex);
		for (auto i = server.channels.begin(); i != server.channels.end(); ++i) {
			channelList[std::string(i->ID)] = *i;
	        	getSettings(this, *i, server.ID);
		}
	} while (false);
	for (auto i = server.members.begin(); i != server.members.end(); ++i) {
		std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
		usercache.push(i->user);
	}
}

void SaveCachedUsers() {
	SleepyDiscord::User u;
	while (true) {
		if (!usercache.empty()) {
			do {
				std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
				u = usercache.front();
				usercache.pop();
			} while (false);
			std::string userid = u.ID;
			std::string bot = u.bot ? "1" : "0";
			db::query("REPLACE INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?)", {userid, u.username, u.discriminator, bot});
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void Bot::onMember(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, SleepyDiscord::ServerMember member) {
	std::string userid = member.user.ID;
	std::string bot = member.user.bot ? "1" : "0";
	db::query("REPLACE INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?)", {userid, member.user.username, member.user.discriminator, bot});
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
		std::thread userqueue(SaveCachedUsers);
	
		try {
			client.run();
		}
		catch (SleepyDiscord::ErrorCode e) {
			std::cout << "Oof! #" << e << std::endl;
		}

		::sleep(5);
	}
}

