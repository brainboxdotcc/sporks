#include "../modules.h"
#include "../regex.h"
#include <string>
#include <cstdint>
#include <fstream>
#include <streambuf>
#include "../stringops.h"
#include "../help.h"

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
		std::string version = "$ModVer 4$";
		return "1.0." + version.substr(8,version.length - 9);
	}

	virtual std::string GetDescription()
	{
		return "Help Commands, '@Sporks help'";
	}

	virtual bool OnMessage(const aegis::gateway::events::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions)
	{
		std::vector<std::string> param;
		std::string botusername = bot->user.username;
		aegis::gateway::objects::message msg = message.msg;
		if (mentioned && helpmessage->Match(clean_message, param)) {
			std::string section = "basic";
			if (param.size() > 2) {
				section = param[2];
			}
			GetHelp(section, message.msg.get_channel_id().get(), botusername, bot->user.id.get(), msg.get_user().get_username(), msg.get_user().get_id().get(), true);
			return false;
		}
		return true;
	}

	void GetHelp(const std::string &section, int64_t channelID, const std::string &botusername, int64_t botid, const std::string &author, int64_t authorid, bool dm)
	{
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
				// FIXME: Don't use .get() on the promise here until it's confirmed that promises are fixed and don't cause segfaults!
				bot->core.create_dm_message(dmobj); //.get();
				channel->create_message("<@" + std::to_string(authorid) + ">, please see your DMs for help text.");
				bot->sent_messages += 2;
			}
			catch (const aegis::exception &e) {
				channel->create_message("<@" + std::to_string(authorid) + ">, I couldn't send help text to you via DM. Please check your privacy settings and try again.");
				bot->sent_messages++;
			}
		} else {
			channel->create_message_embed("", embed_json);
		}
	}
};

ENTRYPOINT(HelpModule);

