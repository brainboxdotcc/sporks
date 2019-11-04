#include "bot.h"
#include "help.h"
#include <string>
#include <cstdint>
#include <fstream>
#include <streambuf>
#include "stringops.h"

void GetHelp(Bot* bot, const std::string &section, int64_t channelID, const std::string &botusername, int64_t botid, const std::string &author, int64_t authorid, bool dm) {

	bool found = true;
	json embed_json;
	aegis::channel* channel = bot->core.find_channel(channelID);

	if (!channel) {
		bot->core.log->error("Can't find channel {}!", channelID);
		return;
	}
	
	std::ifstream t("../help/" + section + ".json");
	if (!t) {
		found = dm = false;
		t = std::ifstream("../help/error.json");
	}
	std::string json((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

	json = ReplaceString(json, ":section:" , section);
	json = ReplaceString(json, ":user:", botusername);
	json = ReplaceString(json, ":id:", std::to_string(botid));
	json = ReplaceString(json, ":author:", author);

	try {
		embed_json = json::parse(json);
	}
	catch (const std::exception &e) {
		channel->create_message("<@" + std::to_string(authorid) + ">, herp derp, theres a malformed help file. Please contact a developer on the official support server: https://discord.gg/brainbox");
		bot->sent_messages++;
		bot->core.log->error("Malformed help file {}.json!", section);
		return;
	}

	if (dm) {
		aegis::create_message_t dmobj;
		try {
			dmobj.user_id(authorid).embed(embed_json).nonce(authorid);
			bot->core.create_dm_message(dmobj);
			channel->create_message("<@" + std::to_string(authorid) + ">, please see your DMs for help text.");
			bot->sent_messages += 2;
		}
		catch (const aegis::exception &e) {
			channel->create_message("<@" + std::to_string(authorid) + ">, I couldn't send help text to you via DM. Please check your privacy settings and try again.");
			bot->sent_messages++;
		}
	} else {
		channel->create_message("", embed_json);
	}
}
