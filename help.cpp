#include "bot.h"
#include "help.h"
#include <string>
#include <cstdint>
#include <fstream>
#include <streambuf>
#include "stringops.h"

void GetHelp(Bot* bot, const std::string &section, int64_t channelID, const std::string &botusername, int64_t botid, const std::string &author, int64_t authorid, bool dm) {

	bool found = true;

	std::ifstream t("../help/" + (found ? section : "error") + ".json");
	if (!t) {
		found = dm = false;
	}
	std::string json((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

	json = ReplaceString(json, ":section:" , section);
	json = ReplaceString(json, ":user:", botusername);
	json = ReplaceString(json, ":id:", std::to_string(botid));
	json = ReplaceString(json, ":author:", author);

	nlohmann::json embed_json = nlohmann::json::parse(json);
	aegis::channel* channel = bot->core.find_channel(channelID);
	if (!channel) {
		bot->core.log->error("Can't find channel {}!", channelID);
		return;
	}

	try {
		if (dm) {
			aegis::create_message_t dmobj;
			dmobj.user_id(authorid).embed(embed_json);
			bot->core.create_dm_message(dmobj);
			channel->create_message("<@" + std::to_string(authorid) + ">, please see your DMs for help text.");
		} else {
			channel->create_message("", embed_json);
		}
	}
	catch (std::exception e) { /* FIXME */
		channel->create_message("<@" + std::to_string(authorid) + ">, I can't send you help, as your DMs from me are blocked. Please check this, and try again.");
	}
}
