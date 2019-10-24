#include "sleepy_discord/sleepy_discord.h"
#include "includes.h"
#include "readline.h"
#include <pcre.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include "database.h"
#include "sleepy_discord/rapidjson/rapidjson.h"
#include "sleepy_discord/rapidjson/document.h"
#include "sleepy_discord/rapidjson/istreamwrapper.h"

class QueueItem
{
  public:
	std::string channelID;
	std::string username;
	std::string message;
};

std::queue<QueueItem> inputs;
std::queue<QueueItem> outputs;

std::mutex input_mutex;
std::mutex output_mutex;

rapidjson::Document config;

std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while((pos = subject.find(search, pos)) != std::string::npos) {
		 subject.replace(pos, search.length(), replace);
		 pos += replace.length();
	}
	return subject;
}

void infobot_socket()
{
	int sockfd = 0;
	struct sockaddr_in serv_addr;
	char recvbuffer[32768];
	std::string response;
	std::string coredata;
	while (true) {
		std::cout << "Connecting to infobot via telnet...\n";
		if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) >= 0) {
			memset(&serv_addr, 0, sizeof(serv_addr)); 
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(config["telnetport"].GetInt());
			inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
			if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
				try {
					/* Log into botnix */
					readLine(sockfd, recvbuffer, sizeof(recvbuffer));
					writeLine(sockfd, config["telnetuser"].GetString());
					readLine(sockfd, recvbuffer, sizeof(recvbuffer));
					writeLine(sockfd, config["telnetpass"].GetString());
					readLine(sockfd, recvbuffer, sizeof(recvbuffer));
					while (true) {
						/* Process anything in the inputs queue */
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						QueueItem query;
						bool has_item = false;
						/* Block to encapsulate lock_guard for input queue */
						do {
							std::lock_guard<std::mutex> input_lock(input_mutex);
							if (!inputs.empty()) {
								query = inputs.front();
								inputs.pop();
								has_item = true;
							}
						} while(false);
						if (has_item) {
							writeLine(sockfd, std::string(".DR ") + ReplaceString(query.username, " ", "_") + " Sporks " + query.message);
							readLine(sockfd, recvbuffer, sizeof(recvbuffer));
							std::stringstream response(recvbuffer);
							std::string text;
							bool found;
							response >> found;
							std::getline(response, text);
							readLine(sockfd, recvbuffer, sizeof(recvbuffer));
							coredata = recvbuffer;

							if (found && text != "*NOTHING*") {
								QueueItem resp;
								resp.username = query.username;
								resp.message = text;
								resp.channelID = query.channelID;
								do {
									 std::lock_guard<std::mutex> output_lock(output_mutex);
									 outputs.push(resp);
								} while (false);
							}
						}
					}
				}
				catch (const std::exception &e) {
					std::cout << "Infobot socket: caught connection exception\n";
				}
			} else {
				std::cout << "Infobot socket: connection failure\n";
			}
		} else {
			std::cout << "Infobot socket: creation of file descriptor failed\n";
		}
		::sleep(5);
	}
}

class MyClientClass : public SleepyDiscord::DiscordClient {
	
	pcre* message_match;
	std::vector<SleepyDiscord::Server> serverList;

public:
	using SleepyDiscord::DiscordClient::DiscordClient;

	void setup() {
		this->setShardID(0, 0);

		const char* pcre_error;
		int pcre_error_ofs;

		message_match = pcre_compile("^SporksDev\\s+", PCRE_CASELESS, &pcre_error, &pcre_error_ofs, NULL);

		if (!message_match) {
			std::cout << "Error compiling regex" << std::endl;
			exit(0);
		}
	}

	void onServer(SleepyDiscord::Server server) override {
		serverList.push_back(server);
		std::cout << "Adding server #" << std::string(server.ID) << ": " << server.name << "\n";
	}

	void onMessage(SleepyDiscord::Message message) override {

		std::cout << "<" << message.author.username << "> " << message.content << std::endl;

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
};

void send_responses(MyClientClass* client) {
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		std::queue<QueueItem> done;
		do {
			std::lock_guard<std::mutex> output_lock(output_mutex);
			while (!outputs.empty()) {
				done.push(outputs.front());
				outputs.pop();
			}
		} while (false);
		while (!done.empty()) {
			client->sendMessage(done.front().channelID, done.front().message);
			done.pop();
		}
	}
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
		exit(0);
	}

	MyClientClass client(token, SleepyDiscord::USER_CONTROLED_THREADS);
	client.setup();
	std::thread infobot(infobot_socket);
	std::thread responses(send_responses, &client);
	client.run();
}

