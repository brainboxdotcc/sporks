#include <aegis.hpp>
#include <sstream>
#include <iostream>
#include <ctime>
#include "status.h"
#include "bot.h"
#include "help.h"
#include "regex.h"
#include "stringops.h"

PCRE uptime_days("(\\d+) day");
PCRE uptime_hours("(\\d+) hour");
PCRE uptime_minutes("(\\d+) min");
PCRE uptime_secs("(\\d+) second");

statusfield::statusfield(const std::string &a, const std::string &b) : name(a), value(b) {
}

void ShowStatus(Bot* bot, const std::vector<std::string> &matches, int64_t channelID) {
	std::stringstream s;

	size_t servers = bot->core.get_guild_count();
	size_t users = bot->core.guilds.size();

	QueueStats qs = bot->GetQueueStats();

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
	s << " status\",\"color\":16767488,\"url\":\"https:\\/\\/brainbox.cc\\/sporks\\/\",\"image\":{\"url\":\"https:\\/\\/neuron.brainbox.cc\\/botnix\\/daylearned.php?now=" << time(NULL);
	s << "\"},\"footer\":{\"link\":\"https;\\/\\/www.botnix.org\\/\",\"text\":\"Powered by Botnix 2.0 with the infobot and discord modules\",\"icon_url\":\"https:\\/\\/www.botnix.org\\/images\\/botnix.png\"},\"fields\":[";
	for (int i = 0; statusfields[i].name != ""; ++i) {
		s << "{\"name\":\"" +  statusfields[i].name + "\",\"value\":\"" + statusfields[i].value + "\", \"inline\": true}";
		if (statusfields[i + 1].name != "") {
			s << ",";
		}
	}
	s << "],\"description\":\"\"}";
	//SleepyDiscord::Embed embed(s.str());
	//bot->sendMessage(channelID, "", embed, false);
}
