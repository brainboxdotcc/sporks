#include "sleepy_discord/sleepy_discord.h"
#include "bot.h"
#include "help.h"
#include <string>
#include <cstdint>
#include <fstream>
#include <streambuf>
#include "stringops.h"
#include "sleepy_discord/rapidjson/rapidjson.h"
#include "sleepy_discord/rapidjson/document.h"
#include "sleepy_discord/rapidjson/istreamwrapper.h"

void GetHelp(Bot* bot, const std::string &section, const std::string &channelID, const std::string &botusername, const std::string &botid, const std::string &author, const std::string &authorid, bool dm) {

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
	json = ReplaceString(json, ":id:", botid);
	json = ReplaceString(json, ":author:", author);

	SleepyDiscord::Embed embed(json);
	SleepyDiscord::Channel c = bot->createDirectMessageChannel(authorid).cast();
	try {
		if (dm) {
			bot->sendMessage(c.ID, "", embed, false);
			bot->sendMessage(channelID, "<@" + authorid + ">, please see your DMs for help text.");
		} else {
			bot->sendMessage(channelID, "", embed, false);
		}
	}
	catch (SleepyDiscord::ErrorCode e) {
		bot->sendMessage(channelID, "<@" + authorid + ">, I can't send you help, as your DMs from me are blocked. Please check this, and try again.");
	}
}
