/************************************************************************************
 * 
 * Sporks, the learning, scriptable Discord bot!
 *
 * Copyright 2019 Craig Edwards <support@sporks.gg>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/

#include <string>
#include <mutex>
#include <string>
#include <sstream>
#include <sporks/bot.h>
#include <sporks/regex.h>
#include "queue.h"
#include <sporks/config.h>
#include <sporks/stringops.h>
#include <sporks/modules.h>
#include "infobot.h"

/**
 * Process output queue from botnix to discord, identify status reports and pass them to the status system
 */
void InfobotModule::OutputThread() {
	PCRE statsreply("Since (.+?), there have been (\\d+) modifications and (\\d+) questions. I have been alive for (.+?), I currently know (\\d+)");
	PCRE url_sanitise("^https?://", true);

	while (!this->terminate) {
		try {
			std::deque<QueueItem> done;
			{
				std::lock_guard<std::mutex> output_lock(this->output_mutex);
				while (!outputs.empty()) {
					done.push_back(outputs.front());
					outputs.pop_front();
				}
			};
			while (!done.empty()) {
				json channel_settings;
				{
					std::lock_guard<std::mutex> hash_lock(bot->channel_hash_mutex);
					channel_settings = getSettings(bot, done.front().channelID, done.front().serverID);
				};
				if (done.front().mentioned || settings::IsTalkative(channel_settings)) {
					try {
						std::string message = trim(done.front().message);

						/* Translate IRC actions */
						if (message.substr(0, 8) == "\001ACTION ") {
							message = "*" + message.substr(8, message.length() - 9) + "*";
						}

						/* Sanitise by putting <> around all but the first url in the message to stop embed spam */
						size_t urls_matched = 0;
						std::stringstream ss(message);
						std::string word;
						while (ss) {
							ss >> word;
							if (url_sanitise.Match(word)) {
								if (urls_matched > 0) {
									message = ReplaceString(message, word, "<" + word + ">");
								}
								urls_matched++;
							}
						}
						if (!done.front().tombstone) {
							/* Prevent training the bot with a @everyone or @here message
							 * Note these are still stored as-is in the database as they arent harmful
							 * on other mediums such as IRC.
							 */
							message = ReplaceString(message, "@everyone", "@‎everyone");
							message = ReplaceString(message, "@here", "@‎here");
							message = ReplaceString(message, "<br>", "\n");
							message = ReplaceString(message, "<s>", "|");
							aegis::channel* channel = bot->core.find_channel(done.front().channelID);
							if (channel) {
								channel->create_message(message);
								bot->sent_messages++;
							}
						}
					}
					catch (std::exception e) {
						bot->core.log->error("Can't send message to channel id {}, (talkative={},mentioned={}), error is: {}", done.front().channelID, settings::IsTalkative(channel_settings), done.front().mentioned, e.what());
					}
				}
				done.pop_front();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		catch (std::exception e) {
			bot->core.log->error("Response Oof! {}", e.what());
		}
	}
}

