#pragma once

#include "sleepy_discord/sleepy_discord.h"
#include "database.h"
#include "bot.h"
#include <string>
#include <cstdint>
#include "sleepy_discord/rapidjson/rapidjson.h"
#include "sleepy_discord/rapidjson/document.h"
#include "sleepy_discord/rapidjson/istreamwrapper.h"

rapidjson::Document getSettings(const std::string &channel_id, Bot* bot);
rapidjson::Document getSettings(const SleepyDiscord::Channel& channel);

namespace settings {
        bool IsLearningDisabled(const rapidjson::Document& settings);
        bool IsLearningEnabled(const rapidjson::Document& settings);
        bool IsTalkative(const rapidjson::Document& settings);
        std::vector<uint64_t> GetIgnoreList(const rapidjson::Document& settings);
};
