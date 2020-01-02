/************************************************************************************
 * 
 * Sporks, the learning, scriptable Discord bot!
 *
 * Copyright 2019 Craig Edwards <support@sporks.gg>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/


#pragma once
#include <aegis.hpp>
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

