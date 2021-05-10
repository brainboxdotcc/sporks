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

#include <sstream>
#include <iostream>
#include <ctime>
#include <sporks/bot.h>
#include <sporks/regex.h>
#include <sporks/stringops.h>
#include <sporks/statusfield.h>
#include <fmt/format.h>
#include <dpp/nlohmann/json.hpp>
#include "infobot.h"

using json = nlohmann::json;

/**
 * Report status to discord as a pretty embed
 */
void InfobotModule::ShowStatus(int days, int hours, int minutes, int seconds, uint64_t db_changes, uint64_t questions, uint64_t facts, time_t startup, int64_t channelID) {
	std::stringstream s;

	uint64_t servers = dpp::get_guild_cache()->count();
	uint64_t users = dpp::get_user_cache()->count();
	uint64_t members = 0;
	for (auto & s : bot->core->get_shards()) {
		members += s.second->GetMemberCount();
	}


	QueueStats qs = this->GetQueueStats();
	char uptime[32];
	snprintf(uptime, 32, "%d day%s, %02d:%02d:%02d", days, (days != 1 ? "s" : ""), hours, minutes, seconds);

	char startstr[256];
	tm _tm;
	gmtime_r(&startup, &_tm);
	strftime(startstr, 255, "%c", &_tm);

	const statusfield statusfields[] = {
		statusfield("Database Changes", Comma(db_changes)),
		statusfield("Connected Since", startstr),
		statusfield("Questions", Comma(questions)),
		statusfield("Approx. Fact Count", Comma(facts)),
		statusfield("Total Servers", Comma(servers)),
		statusfield("Unique Users", Comma(users)),
		statusfield("Members", Comma(members)),
		statusfield("Queue State", "U:"+Comma(qs.users)+", G:"+Comma(qs.guilds)),
		statusfield("Uptime", std::string(uptime)),
		statusfield("Shards", Comma(bot->core->get_shards().size())),
		statusfield("Test Mode", bot->IsTestMode() ? ":white_check_mark: Yes" : "<:wc_rs:667695516737470494> No"),
		statusfield("Developer Mode", bot->IsDevMode() ? ":white_check_mark: Yes" : "<:wc_rs:667695516737470494> No"),
		statusfield("Member Intent", bot->HasMemberIntents() ? ":white_check_mark: Yes" : "<:wc_rs:667695516737470494> No"),
		statusfield("Bot Version", "4.0"),
		statusfield("Library", "<:D_:830553370792165376> [" + std::string(DPP_VERSION_TEXT) + "](https://github.com/brainboxdotcc/DPP)"),
		statusfield("","")
	};

	s << "{\"title\":\"" << bot->user.username;
	s << " status";
	s << "\",\"color\":16767488,\"url\":\"https:\\/\\/sporks.gg\\/\\/\",\"image\":{\"url\":\"https:\\/\\/sporks.gg\\/graphs\\/daylearned.php?now=" << time(NULL);
	s << "\"},\"footer\":{\"link\":\"https;\\/\\/sporks.gg\\/\",\"text\":\"Powered by Sporks!\",\"icon_url\":\"https:\\/\\/sporks.gg\\/images\\/sporks_2020.png\"},\"fields\":[";
	for (int i = 0; statusfields[i].name != ""; ++i) {
		s << "{\"name\":\"" +  statusfields[i].name + "\",\"value\":\"" + statusfields[i].value + "\", \"inline\": true}";
		if (statusfields[i + 1].name != "") {
			s << ",";
		}
	}
	s << "],\"description\":\"\"}";

	json embed_json;
	try {
		embed_json = json::parse(s.str());
	}
	catch (const std::exception &e) {
		bot->core->log(dpp::ll_error, fmt::format("Malformed json created when reporting status: {}", s.str()));
	}
	dpp::channel* channel = dpp::find_channel(channelID);
	if (channel) {
		if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == channel->guild_id) {
			dpp::message m;
			m.channel_id = channelID;
			m.embeds.push_back(&embed_json);
			bot->core->message_create(m, [this](const dpp::confirmation_callback_t & state) {
			});
			bot->sent_messages++;
		}
	} else {
		bot->core->log(dpp::ll_error, fmt::format("Invalid channel id for status: {}", channelID));
	}
}

