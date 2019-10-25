#include "sleepy_discord/sleepy_discord.h"
#include "bot.h"
#include "includes.h"
#include "readline.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include "database.h"
#include "queue.h"
#include "infobot.h"
#include "response.h"
#include "config.h"
#include "regex.h"
#include "stringops.h"

Queue inputs;
Queue outputs;

std::mutex input_mutex;
std::mutex output_mutex;
std::mutex channel_hash_mutex;

rapidjson::Document config;

void Bot::setup() {
	this->setShardID(0, 0);
	message_match = new PCRE("^SporksDev (\\d) (\\d)", true);
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
	}
	while (false);
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
				std::cout << "Bot was mentioned!\n";
				mentioned = true;
			}
		}

		std::cout << "<" << message.author.username << "> " << mentions_removed << std::endl;

		std::string botusername = this->user.username;

		/* Remove bot's nickname from start of message, if it's there */
		while (mentions_removed.substr(0, botusername.length()) == botusername) {
			mentions_removed = trim(mentions_removed.substr(botusername.length(), mentions_removed.length()));
		}

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
	
		std::vector<std::string> m;
		if (message_match->Match(message.content, m)) {
			sendMessage(message.channelID, "Matched regex m[1]=" + m[1] + " m[2]=" + m[2]);
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
		::sleep(2);
		Bot client(token, SleepyDiscord::USER_CONTROLED_THREADS);
		client.setup();
		std::thread infobot(infobot_socket, &client, &input_mutex, &output_mutex, &channel_hash_mutex, &inputs, &outputs, &config);
		std::thread responses(send_responses, &client, &output_mutex, &channel_hash_mutex, &outputs);
	
		try {
			client.run();
		}
		catch (SleepyDiscord::ErrorCode e) {
			std::cout << "Oof! #" << e << std::endl;
		}
	}
}

