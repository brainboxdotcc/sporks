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

#include <dpp/dpp.h>
#include <fmt/format.h>
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
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include "infobot.h"

/**
 * Process output queue from botnix to discord, identify status reports and pass them to the status system
 */
void InfobotModule::Output(QueueItem &done) {
	PCRE statsreply("Since (.+?), there have been (\\d+) modifications and (\\d+) questions. I have been alive for (.+?), I currently know (\\d+)");
	PCRE url_sanitise("^https?://", true);

	json channel_settings;
	channel_settings = getSettings(bot, done.channelID, done.serverID);

	if (done.mentioned || settings::IsTalkative(channel_settings)) {
		try {
			std::string message = trim(done.message);
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
			/* Prevent training the bot with a @everyone or @here message
			 * Note these are still stored as-is in the database as they arent harmful
			 * on other mediums such as IRC.
			 */
			message = ReplaceString(message, "@", "@‎");
			message = ReplaceString(message, "<br>", "\n");
			message = ReplaceString(message, "<s>", "|");
			dpp::channel* channel = dpp::find_channel(done.channelID);
			if (channel) {
				if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == done.serverID) {
					// this doesnt make it to here. fix it later.
					bot->core->log(dpp::ll_info, fmt::format("<{}> {}", done.original_username, done.original_message));
					bot->core->log(dpp::ll_info, fmt::format("<{} ({}/{})> {}", bot->user.username, done.serverID, done.channelID, message));
					bot->core->message_create(dpp::message(channel->id, message));
					bot->sent_messages++;
				}
			}
		}
		catch (const std::exception &e) {
			bot->core->log(dpp::ll_error, fmt::format("Can't send message to channel id {}, (talkative={},mentioned={}), error is: {}", done.channelID, settings::IsTalkative(channel_settings), done.mentioned, e.what()));
		}
	}
}

