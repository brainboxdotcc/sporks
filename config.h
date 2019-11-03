#pragma once

#include <aegis.hpp>
#include "database.h"
#include <string>
#include <cstdint>

using json = nlohmann::json;

json getSettings(class Bot* bot, int64_t channel_id, int64_t guild_id);

namespace settings {
	/* Create json from a simple std::initializer_list of string pairs */
        const std::string createJSON(std::initializer_list<std::pair<std::string, std::string>> json);
	/* Helper functions for types */
        const std::string string(const std::string& s);
        const std::string UInteger(const uint64_t num);
        const std::string optionalUInteger(const uint64_t num);
        const std::string integer(const int64_t num);
        const std::string optionalInteger(const int64_t num);
        const std::string boolean(const bool boolean);
        bool IsLearningDisabled(const json& settings);
        bool IsLearningEnabled(const json& settings);
        bool IsTalkative(const json& settings);
	std::vector<uint64_t> GetIgnoreList(const json& settings);
}

void DoConfig(class Bot* bot, const std::vector<std::string> &param, int64_t channelID, const aegis::gateway::objects::message &message);
bool HasPermission(class Bot* bot, int64_t channelID, const aegis::gateway::objects::message &message);
