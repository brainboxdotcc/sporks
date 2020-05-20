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

#include <sporks/modules.h>
#include <sporks/bot.h>
#include <sporks/config.h>
#include <sporks/database.h>
#include <sporks/stringops.h>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <stdlib.h>
#include <sporks/statusfield.h>
#include <sporks/regex.h>

/**
 * Provides config commands on channels to authorised users
 */

class ConfigModule : public Module
{
	PCRE* configmessage;
public:
	ConfigModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml)
	{
		ml->Attach({ I_OnMessage }, this);
		configmessage = new PCRE("^config(|\\s+(.+?))$", true);
	}

	virtual ~ConfigModule()
	{
		delete configmessage;
	}

	virtual std::string GetVersion()
	{
		/* NOTE: This version string below is modified by a pre-commit hook on the git repository */
		std::string version = "$ModVer 17$";
		return "1.0." + version.substr(8,version.length() - 9);
	}

	virtual std::string GetDescription()
	{
		return "Config Commands, '@Sporks config'";
	}

	/**
	 * Returns true if the user is a server owner, or has manage messages or admin permission on the server.
	 */
	bool HasPermission(int64_t channelID, const aegis::gateway::objects::message &message) {
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

	/**
	 * Config set command
	 */
	void DoConfigSet(std::stringstream &param, int64_t channelID, const aegis::gateway::objects::user& issuer) {
		std::string variable;
		std::string setting;
		param >> variable >> setting;
		variable = lowercase(variable);
		setting = lowercase(setting);
		if (variable == "" || setting == "") {
			EmbedSimple("Missing parameters for config command, please see [the wiki](https://github.com/brainboxdotcc/sporks/wiki/Configuration)", channelID);
			return;
		}
		if (variable != "talkative" && variable != "learn") {
			EmbedSimple("Missing parameters for config command, please see [the wiki](https://github.com/brainboxdotcc/sporks/wiki/Configuration)", channelID);
			return;
		}
		bool state = (setting == "yes" || setting == "true" || setting == "on" || setting == "1");
		json csettings = getSettings(bot, channelID, 0);
	
		bool talkative = (variable == "talkative" ? state : settings::IsTalkative(csettings));
		bool learningdisabled = (variable == "learn" ? !state : settings::IsLearningDisabled(csettings));
	
		json j;
		j["talkative"] = talkative;
		j["learningdisabled"] = learningdisabled;
		j["ignores"] = settings::GetIgnoreList(csettings);
	
		db::query("UPDATE infobot_discord_settings SET settings = '?' WHERE id = ?", {j.dump(), channelID});
	
		EmbedSimple("Setting **'" + variable + "'** " + (state ? "enabled" : "disabled") + " on <#" + std::to_string(channelID) + ">", channelID);
	}
	
	
	/**
	 *  Add, amend and show channel ignore list
	 */
	void DoConfigIgnore(std::stringstream &param, int64_t channelID, const aegis::gateway::objects::message &message) {
		json csettings = getSettings(bot, channelID, 0);
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
			EmbedSimple("You need to refer to the users to add or remove using mentions.", channelID);
			return;
		}
	
		/* Add ignore entries */
		if (operation == "add") {
			for (auto i = mentions.begin(); i != mentions.end(); ++i) {
				if (*i != message.author.id) {
					currentlist.push_back(*i);
				} else {
					EmbedSimple("Foolish human, you can't ignore yourself!", channelID);
					return;
				}
			}
			json j;
			j["talkative"] = settings::IsTalkative(csettings);
			j["learningdisabled"] = settings::IsLearningDisabled(csettings);
			j["ignores"] = currentlist;
			db::query("UPDATE infobot_discord_settings SET settings = '?' WHERE id = ?", {j.dump(), channelID});
			EmbedSimple(std::string("Added **") + std::to_string(mentions.size()) + " user" + (mentions.size() > 1 ? "s" : "") + "** to the ignore list for <#" + std::to_string(channelID) + ">: " + userlist, channelID);
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
			json j;
			j["talkative"] = settings::IsTalkative(csettings);
			j["learningdisabled"] = settings::IsLearningDisabled(csettings);
			j["ignores"] = currentlist;
			db::query("UPDATE infobot_discord_settings SET settings = '?' WHERE id = ?", {j.dump(), channelID});
			EmbedSimple(std::string("Deleted **") + std::to_string(mentions.size()) + " user" + (mentions.size() > 1 ? "s" : "") + "** from the ignore list for <#" + std::to_string(channelID) + ">: " + userlist, channelID);
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
			EmbedSimple(s.str(), channelID);
		}
	}
	
	/**
	 * Show current channel configuration
	 */
	void DoConfigShow(int64_t channelID, const aegis::gateway::objects::user &issuer) {
		json csettings = getSettings(bot, channelID, 0);
		json embed_json;
		std::stringstream s;
	
		const statusfield statusfields[] = {
			statusfield("Talk without being mentioned?", settings::IsTalkative(csettings) ? "Yes" : "No"),
			statusfield("Learn from this channel?", settings::IsLearningEnabled(csettings) ? "Yes" : "No"),
			statusfield("Ignored users", Comma(settings::GetIgnoreList(csettings).size())),
			statusfield("", "")
		};
		s << "{\"title\":\"Settings for this channel\",\"color\":16767488,";
		s << "\"footer\":{\"text\":\"Powered by Sporks!\",\"icon_url\":\"https://sporks.gg/images/sporks_2020.png\"},\"fields\":[";
		for (int i = 0; statusfields[i].name != ""; ++i) {
			s << "{\"name\":\"" +  statusfields[i].name + "\",\"value\":\"" + statusfields[i].value + "\", \"inline\": false}";
			if (statusfields[i + 1].name != "") {
				s << ",";
			}
		}
		s << "],\"description\":\"For help on changing these settings, please see [the wiki](https://github.com/brainboxdotcc/sporks/wiki/Configuration)\"}";
		try {
			embed_json = json::parse(s.str());
		}
		catch (const std::exception &e) {
			bot->core.log->error("Config show for channel ID {}, invalid json: {}", channelID, s.str());
		}
		aegis::channel* channel = bot->core.find_channel(channelID);
		if (channel) {
			if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == channel->get_guild().get_id()) {
				channel->create_message_embed("", embed_json);
			}
		} else {
			bot->core.log->error("Invalid channel {} passed to EmbedSimple", channelID);
		}	
	}

	/**
	 * Main handler called by OnMessage().
	 */
	void DoConfig(const std::vector<std::string> &param, int64_t channelID, const aegis::gateway::objects::message& message) {

		if (!HasPermission(channelID, message)) {
			EmbedSimple("Access denied: You need to be a server owner, have the 'administrator' permission. or have the 'manage messages' permission on this channel to edit its configuration.", channelID);
			return;
		}

		if (param.size() < 3 || param[2] == "") {
			EmbedSimple(std::string("Missing parameters for config command, please see ``@") + bot->user.username + " help config``", channelID);
			return;
		}
		try {
			std::stringstream tokens(trim(param[2]));
			std::string subcommand;
			tokens >> subcommand;

			if (subcommand == "show") {
				DoConfigShow(channelID, message.author);
			} else if (subcommand == "ignore") {
				DoConfigIgnore(tokens, channelID, message);
			} else if (subcommand == "set") {
				DoConfigSet(tokens, channelID, message.author);
			} else {
				EmbedSimple(std::string("Missing parameters for config command, please see ``@") + bot->user.username + " help config``", channelID);
			}
		}
		catch (const std::exception &e) {
			bot->core.log->error("Error processing configuration: {}", e.what());
			EmbedSimple("An internal error occured. If this keeps happening, you can get help on my support server: https://discord.gg/brainbox", channelID);
		}
	}

	/**
	 * Uses a regular expression to identify the config command
	 */
	virtual bool OnMessage(const modevent::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions)
	{
		std::vector<std::string> param;
		aegis::gateway::objects::message msg = message.msg;
		if (mentioned && configmessage->Match(clean_message, param)) {
			bot->core.log->info("CMD: <{}> {}", msg.get_user().get_username(), clean_message);
			DoConfig(param, msg.get_channel_id().get(), msg);
			return false;
		}
		return true;
	}
};

ENTRYPOINT(ConfigModule);

