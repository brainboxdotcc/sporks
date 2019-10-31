#include "bot.h"
#include "config.h"
#include "database.h"
#include "stringops.h"
#include "help.h"
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <stdlib.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

std::mutex config_sql_mutex;

void DoConfigSet(class Bot* bot, std::stringstream &param, int64_t channelID, const aegis::gateway::objects::user &issuer);
void DoConfigIgnore(class Bot* bot, std::stringstream &param, int64_t channelID, const aegis::gateway::objects::message &message);
void DoConfigShow(class Bot* bot, int64_t channelID, const aegis::gateway::objects::user &issuer);

namespace settings {
	const std::string createJSON(std::initializer_list<std::pair<std::string, std::string>> json) {
		std::string target;
		target.reserve(2);
		for (std::pair<std::string, std::string> pair : json) {
			if (pair.second != "") {
				target += ",\"" + pair.first + "\":" + pair.second;
			}
		}
		target[0] = '{';
		target.push_back('}');
		return target;
	}

	const std::string string(const std::string& s) {
		if (s.empty())
			return "";
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		writer.String(s.data(), s.length());
		return std::string(buffer.GetString(), buffer.GetSize());
	}

	const std::string UInteger(const uint64_t num) {
		return std::to_string(num & 0x3FFFFFFFFFFFFF);   //just in case numbers are larger then 53 bits
	}

	const std::string optionalUInteger(const uint64_t num) {
		return num ? UInteger(num) : "";
	}

	const std::string integer(const int64_t num) {
		return std::to_string(num & 0x803FFFFFFFFFFFFF);  //just in case numbers are larger then 53 bits
	}

	const std::string optionalInteger(const int64_t num) {
		return num ? integer(num) : "";
	}

	const std::string boolean(const bool boolean) {
		return boolean ? "true" : "false";
	}
}


rapidjson::Document getSettings(Bot* bot, int64_t channel_id, int64_t guild_id)
{
	std::lock_guard<std::mutex> sql_lock(config_sql_mutex);
	rapidjson::Document settings;
	std::string cid = std::to_string(channel_id);

	aegis::channel* channel = bot->core.find_channel(channel_id);

	if (!channel) {
		std::cout << "WTF, find_channel returned nullptr!\n";
		settings.Parse("{}");
		return settings;
	}

	/* DM channel */
	if (channel->get_type() == 1) {
		settings.Parse("{}");
		return settings;
	}

	/* Retrieve from db */
	db::resultset r = db::query("SELECT settings, parent_id, name FROM infobot_discord_settings WHERE id = ?", {cid});

	/*std::string parent_id = std::to_string(channel->parent_id.get());
	  FIXME doesnt aegis have parent_id?!
		https://docs.aegisbot.io/structaegis_1_1gateway_1_1objects_1_1channel.html
		parent_id is here, in the channel object type from GUILD_CREATE. We may have to cache it.
	 * */
	std::string parent_id = "";

	std::string name = channel->get_name();
	if (parent_id == "") {
		parent_id = "NULL";
	}

	if (channel->get_type() == 0) {	/* Channel type: TEXT */
		name = std::string("#") + name;
	}

	if (r.empty()) {
		/* No settings for this channel, create an entry */
		db::query("INSERT INTO infobot_discord_settings (id, parent_id, guild_id, name, settings) VALUES(?, ?, ?, '?', '?')", {cid, parent_id, std::to_string(guild_id), name, std::string("{}")});
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

	std::string GetIgnoreList(const rapidjson::Document& settings, bool as_json) {
		std::string json = "[";
		if (settings.IsObject() && settings.HasMember("ignores") && settings["ignores"].IsArray()) {
			for (size_t i = 0; i < settings["ignores"].Size(); ++i) {
				json.append(std::to_string(settings["ignores"][i].GetInt64()));
				if (i != settings["ignores"].Size() - 1) {
					json.append(",");
				}
			}
		}
		json.append("]");
		return json;
	}
};

void DoConfig(class Bot* bot, const std::vector<std::string> &param, int64_t channelID, const aegis::gateway::objects::message& message) {

	if (!HasPermission(bot, channelID, message)) {
		GetHelp(bot, "access-denied", channelID, bot->user.username, bot->getID(), message.author.username, message.author.id.get(), false);
		return;
	}

	if (param.size() < 3) {
		GetHelp(bot, "missing-parameters", channelID, bot->user.username, bot->getID(), message.author.username, message.author.id.get(), false);
		return;
	}
	std::stringstream tokens(trim(param[2]));
	std::string subcommand;
	tokens >> subcommand;
	if (subcommand == "show") {
		DoConfigShow(bot, channelID, message.author);
	} else if (subcommand == "ignore") {
		DoConfigIgnore(bot, tokens, channelID, message);
	} else if (subcommand == "set") {
		DoConfigSet(bot, tokens, channelID, message.author);
	}
}

void EmbedSimple(Bot* bot, const std::string &message, int64_t channelID) {
	std::stringstream s;
	s << "{\"color\":16767488, \"description\": \"" << message << "\"}";
	//SleepyDiscord::Embed embed(s.str());
	//bot->sendMessage(channelID, "", embed, false);
}

void DoConfigSet(class Bot* bot, std::stringstream &param, int64_t channelID, const aegis::gateway::objects::user& issuer) {
	std::string variable;
	std::string setting;
	param >> variable >> setting;
	variable = lowercase(variable);
	setting = lowercase(setting);
	if (variable == "" || setting == "") {
		GetHelp(bot, "missing-set-var-or-value", channelID, bot->user.username, bot->getID(), issuer.username, issuer.id.get(), false);
		return;
	}
	if (variable != "talkative" && variable != "learn") {
		GetHelp(bot, "invalid-set-var-or-value", channelID, bot->user.username, bot->getID(), issuer.username, issuer.id.get(), false);
		return;
	}
	bool state = (setting == "yes" || setting == "true" || setting == "on" || setting == "1");
	rapidjson::Document csettings = getSettings(bot, channelID, 0);

	bool talkative = (variable == "talkative" ? state : settings::IsTalkative(csettings));
	bool learningdisabled = (variable == "learn" ? !state : settings::IsLearningDisabled(csettings));
	bool outstate = state;
	if (variable == "learn") {
		outstate = !state;
	}

	std::string json = settings::createJSON({
		{ "talkative", settings::boolean(talkative) },
		{ "learningdisabled", settings::boolean(learningdisabled) },
		{ "ignores", settings::GetIgnoreList(csettings, true) }
	});

	db::query("UPDATE infobot_discord_settings SET settings = '?' WHERE id = ?", {json, std::to_string(channelID)});

	EmbedSimple(bot, "Setting **'" + variable + "'** " + (outstate ? "enabled" : "disabled") + " on <#" + std::to_string(channelID) + ">", channelID);
}

std::string ToJSON(std::vector<uint64_t> list) {
	std::string ret = "[";
	for (auto i = list.begin(); i != list.end(); ++i) {
		ret += std::to_string(*i) + ",";
	}
	ret = ret.substr(0, ret.length() - 1) + "]";
	return ret;
}

bool HasPermission(class Bot* bot, int64_t channelID, const aegis::gateway::objects::message &message) {
	aegis::guild* g = bot->core.find_guild(message.get_guild_id());
	if (g) {
		if (g->get_owner() == message.author.id) {
			/* Server owner */
			return true;
		}
		/* Has manage messages or admin permissions */
		aegis::permission perms = g->get_permissions(bot->core.find_user(message.author.id), bot->core.find_channel(channelID));
		return (perms.can_manage_messages() || perms.is_admin());
	}
	return false;
}

/* Add, amend and show channel ignore list */
void DoConfigIgnore(class Bot* bot, std::stringstream &param, int64_t channelID, const aegis::gateway::objects::message &message) {
	rapidjson::Document csettings = getSettings(bot, channelID, 0);
	std::string operation;
	param >> operation;
	std::string userlist;
	std::vector<uint64_t> currentlist = settings::GetIgnoreList(csettings);
	std::vector<uint64_t> mentions;
	for (auto i = message.mentions.begin(); i != message.mentions.end(); ++i) {
		if (*i != bot->user.id) {
			mentions.push_back(i->get());
			userlist += " " + bot->core.find_user(*i)->get_username();
		}
	}

	if (mentions.size() == 0 && operation != "list") {
		EmbedSimple(bot, "You need to refer to the users to add or remove using mentions.", channelID);
		return;
	}

	/* Add ignore entries */
	if (operation == "add") {
		for (auto i = mentions.begin(); i != mentions.end(); ++i) {
			if (*i != message.author.id) {
				currentlist.push_back(*i);
			} else {
				EmbedSimple(bot, "Foolish human, you can't ignore yourself!", channelID);
				return;
			}
		}
		std::string json = settings::createJSON({
			{ "talkative", settings::boolean(settings::IsTalkative(csettings)) },
			{ "learningdisabled", settings::boolean(settings::IsLearningDisabled(csettings)) },
			{ "ignores", ToJSON(currentlist) },
		});
		db::query("UPDATE infobot_discord_settings SET settings = '?' WHERE id = ?", {json, std::to_string(channelID)});
		EmbedSimple(bot, std::string("Added **") + std::to_string(mentions.size()) + " user" + (mentions.size() > 1 ? "s" : "") + "** to the ignore list for <#" + std::to_string(channelID) + ">: " + userlist, channelID);
	} else if (operation == "del") {
		/* Remove ignore entries */
		std::vector<uint64_t> newlist;
		for (auto i = currentlist.begin(); i != currentlist.end(); ++i) {
			bool preserve = true;
			for (auto j = mentions.begin(); j != mentions.end(); ++j) {
				if (*j == *i) {
					preserve = false;
					break;
				}
			}
			if (preserve) {
				newlist.push_back(*i);
			}
		}
		currentlist = newlist;
		std::string json = settings::createJSON({
			{ "talkative", settings::boolean(settings::IsTalkative(csettings)) },
			{ "learningdisabled", settings::boolean(settings::IsLearningDisabled(csettings)) },
			{ "ignores", ToJSON(currentlist) },
		});
		db::query("UPDATE infobot_discord_settings SET settings = '?' WHERE id = ?", {json, std::to_string(channelID)});
		EmbedSimple(bot, std::string("Deleted **") + std::to_string(mentions.size()) + " user" + (mentions.size() > 1 ? "s" : "") + "** from the ignore list for <#" + std::to_string(channelID) + ">: " + userlist, channelID);
	} else if (operation == "list") {
		/* List ignore entries */
		std::stringstream s;
		if (currentlist.empty()) {
			s << "**Ignore list for <#" << channelID << "> is empty!**";
		} else {
			s << "**Ignore list for <#" << channelID << ">**\\n\\n";
			for (auto i = currentlist.begin(); i != currentlist.end(); ++i) {
				s << "<@" << *i << "> (" << *i << ")\\n";
			}
		}
		EmbedSimple(bot, s.str(), channelID);
	}
}

/* Show current channel configuration */
void DoConfigShow(class Bot* bot, int64_t channelID, const aegis::gateway::objects::user &issuer) {
	rapidjson::Document csettings = getSettings(bot, channelID, 0);
	std::stringstream s;
	const statusfield statusfields[] = {
		statusfield("Talk without being mentioned?", settings::IsTalkative(csettings) ? "Yes" : "No"),
		statusfield("Learn from this channel?", settings::IsLearningEnabled(csettings) ? "Yes" : "No"),
		statusfield("Ignored users", Comma(settings::GetIgnoreList(csettings).size())),
		statusfield("", "")
	};
	s << "{\"title\":\"Settings for this channel\",\"color\":16767488,";
	s << "\"footer\":{\"link\":\"https;\\/\\/www.botnix.org\\/\",\"text\":\"Powered by Botnix 2.0 with the infobot and discord modules\",\"icon_url\":\"https:\\/\\/www.botnix.org\\/images\\/botnix.png\"},\"fields\":[";
	for (int i = 0; statusfields[i].name != ""; ++i) {
		s << "{\"name\":\"" +  statusfields[i].name + "\",\"value\":\"" + statusfields[i].value + "\", \"inline\": false}";
		if (statusfields[i + 1].name != "") {
			s << ",";
		}
	}
	s << "],\"description\":\"For help on changing these settings, type ``@" << bot->user.username << " help config``.\"}";
	//SleepyDiscord::Embed embed(s.str());
	//bot->sendMessage(channelID, "", embed, false);
}

