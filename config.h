#pragma once

#include "sleepy_discord/sleepy_discord.h"
#include "database.h"
#include <string>
#include <cstdint>
#include "sleepy_discord/rapidjson/rapidjson.h"
#include "sleepy_discord/rapidjson/document.h"
#include "sleepy_discord/rapidjson/istreamwrapper.h"

rapidjson::Document getSettings(class Bot* bot, const std::string &channel_id, const std::string& guild_id);
rapidjson::Document getSettings(class Bot* bot, const SleepyDiscord::Channel& channel, const std::string& guild_id);

namespace settings {
        bool IsLearningDisabled(const rapidjson::Document& settings);
        bool IsLearningEnabled(const rapidjson::Document& settings);
        bool IsTalkative(const rapidjson::Document& settings);
        std::vector<uint64_t> GetIgnoreList(const rapidjson::Document& settings);
};
