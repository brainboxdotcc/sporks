#include "sleepy_discord/sleepy_discord.h"
#include "bot.h"
#include "includes.h"
#include "readline.h"
#include <pcre.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include "database.h"
#include "queue.h"
#include "infobot.h"
#include "response.h"
#include "config.h"

Queue inputs;
Queue outputs;

std::mutex input_mutex;
std::mutex output_mutex;

rapidjson::Document config;

void Bot::setup() {
	this->setShardID(0, 0);

	const char* pcre_error;
	int pcre_error_ofs;

	message_match = pcre_compile("^SporksDev\\s+", PCRE_CASELESS, &pcre_error, &pcre_error_ofs, NULL);
	if (!message_match) {
		std::cout << "Error compiling regex" << std::endl;
		exit(0);
	}
}

void Bot::onServer(SleepyDiscord::Server server) {
	serverList.push_back(server);
	std::cout << "Adding server #" << std::string(server.ID) << ": " << server.name << "\n";
	for (auto i = server.channels.begin(); i != server.channels.end(); ++i) {
		this->onChannel(*i);
	}
}

void Bot::onMessage(SleepyDiscord::Message message) {

	std::cout << "<" << message.author.username << "> " << message.content << std::endl;

	rapidjson::Document settings = getSettings(message.channelID, this);

	if (message.author.ID != this->getID()) {
		do {
			std::lock_guard<std::mutex> input_lock(input_mutex);
			QueueItem query;
			query.message = message.content;
			query.channelID = message.channelID;
			query.username = message.author.username;
			inputs.push(query);
		} while (false);
	
		if (pcre_exec(message_match, NULL, message.content.c_str(), message.content.length(), 0, 0, NULL, 0) > -1) {
			sendMessage(message.channelID, "Matched regex");
		}
	}
}

void Bot::onChannel(SleepyDiscord::Channel channel) {
	getSettings(channel);
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

	Bot client(token, SleepyDiscord::USER_CONTROLED_THREADS);
	client.setup();
	std::thread infobot(infobot_socket, &input_mutex, &output_mutex, &inputs, &outputs, &config);
	std::thread responses(send_responses, &client, &output_mutex, &outputs);
	client.run();
}

