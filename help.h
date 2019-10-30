#pragma once

#include <string>

struct statusfield {
	std::string name;
	std::string value;
	statusfield(const std::string &a, const std::string &b);
};

void GetHelp(class Bot* bot, const std::string &section, int64_t channelID, const std::string &botusername, int64_t botid, const std::string &author, int64_t authorid, bool dm = true);

