#include <aegis.hpp>
#include <sstream>
#include <iostream>
#include <ctime>
#include <sporks/bot.h>
#include <sporks/regex.h>
#include <sporks/stringops.h>
#include <sporks/statusfield.h>
#include "infobot.h"

PCRE uptime_days("(\\d+) day");
PCRE uptime_hours("(\\d+) hour");
PCRE uptime_minutes("(\\d+) min");
PCRE uptime_secs("(\\d+) second");

void InfobotModule::ShowStatus(const std::vector<std::string> &matches, int64_t channelID) {
	std::stringstream s;

	int64_t servers = bot->core.get_guild_count();
	int64_t users = bot->core.get_member_count();

	QueueStats qs = this->GetQueueStats();

	std::vector<std::string> m;
	int days = 0, hours = 0, minutes = 0, seconds = 0;
	if (uptime_days.Match(matches[4], m)) {
		days = from_string<int>(m[1], std::dec);
	}
	if (uptime_hours.Match(matches[4], m)) {
		hours = from_string<int>(m[1], std::dec);
	}
	if (uptime_minutes.Match(matches[4], m)) {
		minutes = from_string<int>(m[1], std::dec);
	}
	if (uptime_secs.Match(matches[4], m)) {
		seconds = from_string<int>(m[1], std::dec);
	}
	char uptime[32];
	sprintf(uptime, "%02d days, %02d:%02d:%02d", days, hours, minutes, seconds);

	const statusfield statusfields[] = {
		statusfield("Database Changes", Comma(from_string<size_t>(matches[2], std::dec))),
		statusfield("Connected Since", matches[1]),
		statusfield("Questions", Comma(from_string<size_t>(matches[3], std::dec))),
		statusfield("Facts in database", Comma(from_string<size_t>(matches[5], std::dec))),
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

