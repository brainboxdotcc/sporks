#include <string>
#include <mutex>
#include <string>
#include <sstream>
#include "bot.h"
#include "regex.h"
#include "queue.h"
#include "config.h"
#include "stringops.h"
#include "status.h"

void send_responses(Bot* client, std::mutex* output_mutex, std::mutex* channel_hash_mutex, Queue* outputs) {
	PCRE statsreply("Since (.+?), there have been (\\d+) modifications and (\\d+) questions. I have been alive for (.+?), I currently know (\\d+)");
	PCRE url_sanitise("^https?://", true);
	while (true) {
		try {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			std::queue<QueueItem> done;
			do {
				std::lock_guard<std::mutex> output_lock(*output_mutex);
				while (!outputs->empty()) {
					done.push(outputs->front());
					outputs->pop();
				}
			} while (false);
			while (!done.empty()) {
				SleepyDiscord::Channel channel;
				rapidjson::Document channel_settings;
				do {
					std::lock_guard<std::mutex> hash_lock(*channel_hash_mutex);
					channel = client->channelList.find(done.front().channelID)->second;
					channel_settings = getSettings(client, channel, done.front().serverID);
				} while (false);
				if (done.front().mentioned || settings::IsTalkative(channel_settings)) {
					try {
						std::vector<std::string> m;
						if (statsreply.Match(done.front().message, m)) {
							/* Handle status reply */
							ShowStatus(client, m, done.front().channelID);
						} else {
							/* Anything else */
							std::string message = done.front().message;

							/* Sanitise by putting <> around all but the first url in the message to stop embed spam */
							size_t urls_matched = 0;
							std::stringstream ss(message);
							std::string word;
							while ((ss >> word) != false) {
								if (url_sanitise.Match(word)) {
									if (urls_matched > 0) {
										message = ReplaceString(message, word, "<" + word + ">");
									}
									urls_matched++;
								}
							}
							client->sendMessage(channel.ID, message);
						}
					}
					catch (SleepyDiscord::ErrorCode e) {
						std::cout << "Can't send message to channel id " << std::string(channel.ID) << " (talkative=" << settings::IsTalkative(channel_settings) << ",mentioned=" << done.front().mentioned << ") message was: '" << done.front().message << "'" << std::endl;
					}
				}
				done.pop();
			}
		}
		catch (SleepyDiscord::ErrorCode e) {
			std::cout << "Response Oof! #" << e << std::endl;
		}
	}
}

