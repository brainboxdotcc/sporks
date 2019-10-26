#include "sleepy_discord/sleepy_discord.h"
#include <sstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <locale>
#include "status.h"
#include "bot.h"

template<class T> std::string Comma(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

template <typename T> T from_string(const std::string &s, std::ios_base & (*f)(std::ios_base&))
{
	T t;
	std::istringstream iss(s);
	iss >> f, iss >> t;
	return t;
}

struct statusfield {
	std::string name;
	std::string value;
	statusfield(const std::string &a, const std::string &b) : name(a), value(b) {};
};

void ShowStatus(Bot* bot, const std::vector<std::string> &matches, const std::string &channelID) {
	std::stringstream s;

	size_t servers = bot->serverList.size();
	size_t users = 0;
	for (size_t i = 0; i < bot->serverList.size(); ++i) {
		users += bot->serverList[i].members.size();
	}

	const statusfield statusfields[] = {
		statusfield("Database Changes", Comma(from_string<size_t>(matches[2], std::dec))),
		statusfield("Connected Since", matches[1]),
		statusfield("Questions", Comma(from_string<size_t>(matches[3], std::dec))),
		statusfield("Facts in database", Comma(from_string<size_t>(matches[5], std::dec))),
		statusfield("Total Servers", Comma(servers)),
		statusfield("Online Users", Comma(users)),
		statusfield("Uptime", matches[4]),
		statusfield("","")
	};

	s << "{\"title\":\"" << bot->user.username;
	s << " status\",\"color\":16767488,\"url\":\"https:\\/\\/www.botnix.org\",\"image\":{\"url\":\"https:\\/\\/neuron.brainbox.cc\\/botnix\\/daylearned.php?now=" << time(NULL);
	s << "\"},\"thumbnail\":{\"url\":\"https:\\/\\/www.botnix.org\\/images\\/botnix.png\"},\"footer\":{\"link\":\"https;\\/\\/www.botnix.org\\/\",\"text\":\"Powered by Botnix 2.0 with the infobot and discord modules\",\"icon_url\":\"https:\\/\\/www.botnix.org\\/images\\/botnix.png\"},\"fields\":[";
	for (int i = 0; statusfields[i].name != ""; ++i) {
		s << "{\"name\":\"" +  statusfields[i].name + "\",\"value\":\"" + statusfields[i].value + "\", \"inline\": true}";
		if (statusfields[i + 1].name != "") {
			s << ",";
		}
	}
	s << "],\"description\":\"\"}";
	SleepyDiscord::Embed embed(s.str());
	bot->sendMessage(channelID, "", embed, false);
}
