#include "bot.h"
#include "config.h"
#include "database.h"
#include <string>
#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <mutex>

using namespace SleepyDiscord;

std::mutex config_sql_mutex;

rapidjson::Document getSettings(Bot* bot, const std::string &channel_id, const std::string& guild_id)
{
	Channel channel;

	ChannelCache::iterator iter = bot->channelList.find(channel_id);
	if (iter != bot->channelList.end()) {
		channel = iter->second;
	} else {
		const Channel& c = bot->getChannel(Snowflake<Channel>(channel_id)).cast();
		bot->channelList[channel_id] = c;
		channel = c;
	}

	return getSettings(bot, channel, guild_id);
}

rapidjson::Document getSettings(Bot* bot, const Channel& channel, const std::string& guild_id)
{
	std::lock_guard<std::mutex> sql_lock(config_sql_mutex);
	rapidjson::Document settings;
	std::string cid = std::string(channel.ID);

	/* Retrieve from db */
	db::resultset r = db::query("SELECT settings, parent_id, name FROM infobot_discord_settings WHERE id = ?", {cid});

	std::string parent_id = std::string(channel.parentID);
	std::string name = channel.name;
	if (parent_id == "") {
		parent_id = "NULL";
	}

	if (channel.type == 0) {	/* Channel type: TEXT */
		name = std::string("#") + name;
	}

	if (r.empty()) {
		/* No settings for this channel, create an entry */
		db::query("INSERT INTO infobot_discord_settings (id, parent_id, guild_id, name, settings) VALUES(?, ?, ?, '?', '?')", {cid, parent_id, guild_id, name, std::string("{}")});
		r = db::query("SELECT settings FROM infobot_discord_settings WHERE id = ?", {cid});

	} else if (name != r[0].find("name")->second || parent_id != r[0].find("parent_id")->second) {
		/* Data has changed, run update query */
		db::query("UPDATE infobot_discord_settings SET parent_id = ?, name = '?' WHERE id = ?", {parent_id, name, cid});
	}

	db::row row = r[0];
	std::string json = row.find("settings")->second;
	settings.Parse(json.c_str());

	if (settings.IsObject()) {
		return settings;
	} else {
		settings.Parse("{}");
		return settings;
	}
}

// {"talkative":true,"learningdisabled":false,"ignores":[159985870458322944,155149108183695360]}

namespace settings {

	bool IsLearningDisabled(const rapidjson::Document& settings) {
		return (settings.IsObject() && settings.HasMember("learningdisabled") && settings["learningdisabled"].IsBool() && settings["learningdisabled"].GetBool() == true);
	}

	bool IsLearningEnabled(const rapidjson::Document& settings) {
		return !IsLearningDisabled(settings);
	}

	bool IsTalkative(const rapidjson::Document& settings) {
		return (settings.IsObject() && settings.HasMember("talkative") && settings["talkative"].IsBool() && settings["talkative"].GetBool() == true);
	}

	std::vector<uint64_t> GetIgnoreList(const rapidjson::Document& settings) {
		std::vector<uint64_t> ignores;
		if (settings.IsObject() && settings.HasMember("ignores") && settings["ignores"].IsArray()) {
			for (size_t i = 0; i < settings["ignores"].Size(); ++i) {
				ignores.push_back(settings["ignores"][i].GetInt64());
			}
		}
		return ignores;
	}
};
