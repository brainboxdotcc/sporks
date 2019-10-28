#pragma once

#include <string>

struct statusfield {
	std::string name;
	std::string value;
	statusfield(const std::string &a, const std::string &b);
};

void GetHelp(class Bot* bot, const std::string &section, const std::string &channelID, const std::string &botusername, const std::string &botid, const std::string &author, const std::string &authorid, bool dm = true);

