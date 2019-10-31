#include "bot.h"
#include "help.h"
#include <string>
#include <cstdint>
#include <fstream>
#include <streambuf>
#include "stringops.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

void GetHelp(Bot* bot, const std::string &section, int64_t channelID, const std::string &botusername, int64_t botid, const std::string &author, int64_t authorid, bool dm) {

	bool found = true;
	rapidjson::Document helpdoc;
	std::ifstream helpfile("../help/" + section + ".json");
	rapidjson::IStreamWrapper wrapper(helpfile);
	helpdoc.ParseStream(wrapper);

	if (!helpdoc.IsObject()) {
		found = false;
		dm = false;
	}

	std::ifstream t("../help/" + (found ? section : "error") + ".json");
	std::string json((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

	json = ReplaceString(json, ":section:" , section);
	json = ReplaceString(json, ":user:", botusername);
	json = ReplaceString(json, ":id:", std::to_string(botid));
	json = ReplaceString(json, ":author:", author);

	//SleepyDiscord::Embed embed(json);
	//SleepyDiscord::Channel c = bot->createDirectMessageChannel(authorid).cast();
	try {
		if (dm) {
			//bot->sendMessage(c.ID, "", embed, false);
			//bot->sendMessage(channelID, "<@" + authorid + ">, please see your DMs for help text.");
		} else {
			//bot->sendMessage(channelID, "", embed, false);
		}
	}
	catch (std::exception e) { /* FIXME */
		//bot->sendMessage(channelID, "<@" + std::to_string(authorid) + ">, I can't send you help, as your DMs from me are blocked. Please check this, and try again.");
	}
}
