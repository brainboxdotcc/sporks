#pragma once

#include <aegis.hpp>
#include <sporks/database.h>
#include <string>
#include <cstdint>

using json = nlohmann::json;

/* Get settings for a channel */
json getSettings(class Bot* bot, int64_t channel_id, int64_t guild_id);

namespace settings {
	/* Returns true if learning is disabled */
        bool IsLearningDisabled(const json& settings);
	/* Returns true if learning is enabled */
        bool IsLearningEnabled(const json& settings);
	/* Returns true if the bot is talkative */
        bool IsTalkative(const json& settings);
	/* Returns the ignore list */
	std::vector<uint64_t> GetIgnoreList(const json& settings);
	/* Returns javascript configuration.
	 * FIXME: Move me to js module.
	 */
	std::string getJSConfig(int64_t channel_id, std::string variable);
	/* Sets the JS configuration
	 * FIXME: Move me to js module
	 */
	void setJSConfig(int64_t channel_id, std::string variable, std::string value);
}

