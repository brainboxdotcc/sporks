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

#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <sporks/modules.h>
#include <sporks/regex.h>
#include <string>
#include <cstdint>
#include <fstream>
#include <streambuf>
#include <sporks/stringops.h>
#include <sporks/statusfield.h>

using json = nlohmann::json;

/**
 * Provides help commands from JSON files in the help directory
 */

class HelpModule : public Module
{
	PCRE* helpmessage;
public:
	HelpModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml)
	{
		ml->Attach({ I_OnMessage }, this);
		helpmessage = new PCRE("^help(|\\s+(.+?))$", true);
	}

	virtual ~HelpModule()
	{
		delete helpmessage;
	}

	virtual std::string GetVersion()
	{
		/* NOTE: This version string below is modified by a pre-commit hook on the git repository */
		std::string version = "$ModVer 17$";
		return "1.0." + version.substr(8,version.length() - 9);
	}

	virtual std::string GetDescription()
	{
		return "Help Commands, '@Sporks help'";
	}

	/**
	 * Uses a regular expression to identify the help command and its single parameter
	 */
	virtual bool OnMessage(const dpp::message_create_t &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions)
	{
		std::vector<std::string> param;
		std::string botusername = bot->user.username;
		dpp::message msg = *(message.msg);
		if (mentioned && helpmessage->Match(clean_message, param)) {
			std::string section = "basic";
			if (param.size() > 2) {
				section = param[2];
			}
			GetHelp(section, msg.channel_id, botusername, bot->user.id, msg.author ? msg.author->username : "", msg.author ? msg.author->id : 0, true);
			return false;
		}
		return true;
	}
	
	/**
	 * Emit help using a json file in the help/ directory. Missing help files emit a generic error message.
	 */
	void GetHelp(const std::string &section, int64_t channelID, const std::string &botusername, int64_t botid, const std::string &author, int64_t authorid, bool dm)
	{
		json embed_json;
		char timestamp[256];
		time_t timeval = time(NULL);
		dpp::channel* channel = dpp::find_channel(channelID);

		if (!channel) {
			bot->core->log(dpp::ll_error, fmt::format("Can't find channel {}!", channelID));
			return;
		}
	
		std::ifstream t("../help/" + section + ".json");
		if (!t) {
			t = std::ifstream("../help/error.json");
		}
		std::string _json((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

		tm _tm;
		gmtime_r(&timeval, &_tm);
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &_tm);

		_json = ReplaceString(_json, ":section:" , section);
		_json = ReplaceString(_json, ":user:", botusername);
		_json = ReplaceString(_json, ":id:", std::to_string(botid));
		_json = ReplaceString(_json, ":author:", author);
		_json = ReplaceString(_json, ":ts:", timestamp);
	
		try {
			embed_json = json::parse(_json);
		}
		catch (const std::exception &e) {
			if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == channel->guild_id) {
				bot->core->message_create(dpp::message(channel->id, "<@" + std::to_string(authorid) + ">, herp derp, theres a malformed help file. Please contact a developer on the official support server: https://discord.gg/brainbox"));
				bot->sent_messages++;
			}
			bot->core->log(dpp::ll_error, fmt::format("Malformed help file {}.json!", section));
			return;
		}

		if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == channel->guild_id) {
			dpp::message m;
			m.embeds.push_back(dpp::embed(&embed_json));
			m.channel_id = channel->id;
			bot->core->message_create(m);
			bot->sent_messages++;
		}
	}
};

ENTRYPOINT(HelpModule);

