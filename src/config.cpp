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

#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <sporks/bot.h>
#include <sporks/database.h>
#include <sporks/stringops.h>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <stdlib.h>

std::mutex config_sql_mutex;
using json = nlohmann::json;

namespace settings {

/* Get one configuration variable for a channel by ID */
std::string getJSConfig(int64_t channel_id, std::string variable)
{
	db::resultset r = db::query("SELECT `" + variable + "` FROM infobot_discord_javascript WHERE id = ?", {channel_id});
	if (r.size() == 0) {
		return "";
	} else {
		return r[0].find(variable)->second;
	}
}

/* Set one configuration variable for a channel by ID */
void setJSConfig(int64_t channel_id, std::string variable, std::string value)
{
	db::resultset r = db::query("UPDATE infobot_discord_javascript SET `" + variable + "` = '?' WHERE id = ?", {value, channel_id});
}

};

/**
 * Get all configuration variables for a channel by ID.
 *
 * SIDE EFFECTS:
 * If there are no configuration settings, create blank settings and return an empty set.
 */
json getSettings(Bot* bot, int64_t channel_id, int64_t guild_id)
{
	std::lock_guard<std::mutex> sql_lock(config_sql_mutex);
	json settings;

	dpp::channel* channel = dpp::find_channel(channel_id);

	if (!channel) {
		bot->core->log(dpp::ll_error, fmt::format("WTF, find_channel({}) returned nullptr!", channel_id));
		return settings;
	}

	/* DM channels dont have settings */
	if (channel->is_dm()) {
		return settings;
	}

	/* Retrieve from db */
	db::resultset r = db::query("SELECT settings, parent_id, name FROM infobot_discord_settings WHERE id = ?", {channel_id});

	std::string parent_id = std::to_string(channel->parent_id);
	std::string name = channel->name;

	if (parent_id == "" || parent_id == "0") {
		parent_id = "NULL";
	}

	if (channel->is_text_channel()) {
		name = std::string("#") + name;
	}

	if (r.empty()) {
		/* No settings for this channel, create an entry */
		db::query("INSERT INTO infobot_discord_settings (id, parent_id, guild_id, name, settings) VALUES(?, ?, ?, '?', '?')", {channel_id, parent_id, guild_id, name, std::string("{}")});
		r = db::query("SELECT settings FROM infobot_discord_settings WHERE id = ?", {channel_id});

	} else if (name != r[0].find("name")->second || parent_id != r[0].find("parent_id")->second) {
		/* Data has changed, run update query */
		db::query("UPDATE infobot_discord_settings SET parent_id = ?, name = '?' WHERE id = ?", {parent_id, name, channel_id});
	}

	db::row row = r[0];
	std::string j = row.find("settings")->second;
	try {
		settings = json::parse(j);
	} catch (const std::exception &e) {
		bot->core->log(dpp::ll_error, fmt::format("Can't parse settings for channel {}, id {}, json settings were: {}", channel->name, channel_id, j));
	}

	return settings;
}

namespace settings {

	/**
	 * Returns true if learning is disabled in the given settings
	 */
	bool IsLearningDisabled(const json& settings) {
		return settings.value("learningdisabled", false);
	}

	/**
	 * Returns true if learning is enabled in the given settings
	 */
	bool IsLearningEnabled(const json& settings) {
		return !IsLearningDisabled(settings);
	}

	/**
	 * Returns true if talkative is enabled in the given settings
	 */
	bool IsTalkative(const json& settings) {
		return settings.value("talkative", false);
	}

	/**
	 * Returns a vector of snowflake ids representing the ignore list,
	 * from the given settings.
	 */
	std::vector<uint64_t> GetIgnoreList(const json& settings) {
		std::vector<uint64_t> ignores;
		if (settings.find("ignores") != settings.end()) {
			for (auto i = settings["ignores"].begin(); i != settings["ignores"].end(); ++i) {
				ignores.push_back(i->get<uint64_t>());
			}
		}
		return ignores;
	}
};

