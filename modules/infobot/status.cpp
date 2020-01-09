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

#include <aegis.hpp>
#include <sstream>
#include <iostream>
#include <ctime>
#include <sporks/bot.h>
#include <sporks/regex.h>
#include <sporks/stringops.h>
#include <sporks/statusfield.h>
#include "infobot.h"

/**
 * Report status to discord as a pretty embed
 */
void InfobotModule::ShowStatus(int days, int hours, int minutes, int seconds, uint64_t db_changes, uint64_t questions, uint64_t facts, time_t startup, int64_t channelID) {
	std::stringstream s;

	int64_t servers = bot->core.get_guild_count();
	int64_t users = bot->core.get_member_count();

	QueueStats qs = this->GetQueueStats();
	char uptime[32];
	sprintf(uptime, "%d day%s, %02d:%02d:%02d", days, (days != 1 ? "s" : ""), hours, minutes, seconds);

	char startstr[256];
	tm* _tm;
	_tm = gmtime(&startup);
	strftime(startstr, 255, "%c", _tm);

	const statusfield statusfields[] = {
		statusfield("Database Changes", Comma(db_changes)),
		statusfield("Connected Since", startstr),
		statusfield("Questions", Comma(questions)),
		statusfield("Facts in database", Comma(facts)),
		statusfield("Total Servers", Comma(servers)),
		statusfield("Online Users", Comma(users)),
		statusfield("Input Queue", Comma(qs.inputs)),
		statusfield("Output Queue", Comma(qs.outputs)),
		statusfield("User Queue", Comma(qs.users)),
		statusfield("Uptime", std::string(uptime)),
		statusfield("","")
	};

	s << "{\"title\":\"" << bot->user.username;
	s << " status\",\"color\":16767488,\"url\":\"https:\\/\\/sporks.gg\\/\\/\",\"image\":{\"url\":\"https:\\/\\/sporks.gg\\/graphs\\/daylearned.php?now=" << time(NULL);
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
		bot->core.log->error("Malformed json created when reporting status: {}", s.str());
	}
	aegis::channel* channel = bot->core.find_channel(channelID);
	if (channel) {
		channel->create_message_embed("", embed_json);
		bot->sent_messages++;
	}
}

