#include "bot.h"
#include "includes.h"
#include "readline.h"
#include "queue.h"
#include "infobot.h"
#include "config.h"
#include "stringops.h"
#include <iostream>
#include <sstream>
#include <thread>

void infobot_socket(Bot* client, std::mutex *input_mutex, std::mutex *output_mutex, std::mutex *channel_hash_mutex, Queue *inputs, Queue *outputs, rapidjson::Document* config)
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
			serv_addr.sin_port = htons((*config)["telnetport"].GetInt());
			inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
			if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
				try {
					/* Log into botnix */
					readLine(sockfd, recvbuffer, sizeof(recvbuffer));
					writeLine(sockfd, (*config)["telnetuser"].GetString());
					readLine(sockfd, recvbuffer, sizeof(recvbuffer));
					writeLine(sockfd, (*config)["telnetpass"].GetString());
					readLine(sockfd, recvbuffer, sizeof(recvbuffer));
					std::cout << "Socket link to botnix is UP, and ready for queries" << std::endl;
					while (true) {
						/* Process anything in the inputs queue */
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						QueueItem query;
						bool has_item = false;
						/* Block to encapsulate lock_guard for input queue */
						do {
							std::lock_guard<std::mutex> input_lock(*input_mutex);
							if (!inputs->empty()) {
								query = inputs->front();
								SleepyDiscord::Channel channel;
								rapidjson::Document channel_settings;
								do {
									std::lock_guard<std::mutex> hash_lock(*channel_hash_mutex);
									channel = client->channelList.find(query.channelID)->second;
									channel_settings = getSettings(client, channel);

								} while(false);
								has_item = query.mentioned || settings::IsLearningEnabled(channel_settings);
								if (query.mentioned) {
									std::cout << "mentioned\n";
								}
								if (settings::IsLearningEnabled(channel_settings)) {
									std::cout << "learning enabled\n";
								}
								inputs->pop();
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
									 std::lock_guard<std::mutex> output_lock(*output_mutex);
									 outputs->push(resp);
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
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}
