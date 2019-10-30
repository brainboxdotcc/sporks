#pragma once

#include <aegis.hpp>
#include "database.h"
#include <string>
#include <cstdint>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>


rapidjson::Document getSettings(class Bot* bot, int64_t channel_id, int64_t guild_id);

namespace settings {
        bool IsLearningDisabled(const rapidjson::Document& settings);
        bool IsLearningEnabled(const rapidjson::Document& settings);
        bool IsTalkative(const rapidjson::Document& settings);
	std::vector<uint64_t> GetIgnoreList(const rapidjson::Document& settings);
        std::string GetIgnoreList(const rapidjson::Document& settings, bool as_json);
};

void DoConfig(class Bot* bot, const std::vector<std::string> &param, int64_t channelID, const aegis::gateway::objects::message &message);
bool HasPermission(class Bot* bot, int64_t channelID, const aegis::gateway::objects::message &message);
