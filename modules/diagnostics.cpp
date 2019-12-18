#include "../bot.h"
#include "../regex.h"
#include "../modules.h"
#include "../stringops.h"
#include "../statusfield.h"
#include <sstream>

/**
 * Provides diagnostic commands for monitoring the bot and debugging it interactively while it's running.
 */

class DiagnosticsModule : public Module
{
	PCRE* diagnosticmessage;
public:
	DiagnosticsModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml)
	{
		ml->Attach({ I_OnMessage }, this);
		diagnosticmessage = new PCRE("^sudo(|\\s+(.+?))$", true);
	}

	virtual ~DiagnosticsModule()
	{
		delete diagnosticmessage;
	}

	virtual std::string GetVersion()
	{
		/* NOTE: This version string below is modified by a pre-commit hook on the git repository */
		std::string version = "$ModVer 7$";
		return "1.0." + version.substr(8,version.length() - 9);
	}

	virtual std::string GetDescription()
	{
		return "Diagnostic Commands (sudo), '@Sporks sudo'";
	}

	virtual bool OnMessage(const aegis::gateway::events::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions)
	{
		std::vector<std::string> param;
		std::string botusername = bot->user.username;
		aegis::gateway::objects::message msg = message.msg;

		if (mentioned && diagnosticmessage->Match(clean_message, param)) {

			std::stringstream tokens(trim(param[2]));
			std::string subcommand;
			tokens >> subcommand;

			/* Get owner snowflake id from config file */
			int64_t owner_id = from_string<int64_t>(Bot::GetConfig("owner"), std::dec);

			if (msg.author.id.get() == owner_id) {

				if (param.size() < 3) {
					EmbedSimple("Sudo make me a sandwich.", msg.get_channel_id().get());
				} else {
					if (lowercase(subcommand) == "modules") {
						std::stringstream s;

						// NOTE: GetModuleList's reference is safe from within a module event
						const ModMap& modlist = bot->Loader->GetModuleList();

						s << "```diff" << std::endl;
						s << fmt::format("- ╭─────────────────────────┬───────────┬────────────────────────────────────────────────╮") << std::endl;
						s << fmt::format("- │ Filename                | Version   | Description                                    |") << std::endl;
						s << fmt::format("- ├─────────────────────────┼───────────┼────────────────────────────────────────────────┤") << std::endl;

						for (auto mod = modlist.begin(); mod != modlist.end(); ++mod) {
							s << fmt::format("+ │ {:23} | {:9} | {:46} |", mod->first, mod->second->GetVersion(), mod->second->GetDescription()) << std::endl;
						}
						s << fmt::format("+ ╰─────────────────────────┴───────────┴────────────────────────────────────────────────╯") << std::endl;
						s << "```";

						aegis::channel* c = bot->core.find_channel(msg.get_channel_id().get());
						if (c) {
							c->create_message(s.str());
						}
						
					} else if (lowercase(subcommand) == "load") {
						std::string modfile;
						tokens >> modfile;
						if (bot->Loader->Load(modfile)) {
							EmbedSimple("Loaded module: " + modfile, msg.get_channel_id().get());
						} else {
							EmbedSimple(std::string("Can't do that: ``") + bot->Loader->GetLastError() + "``", msg.get_channel_id().get());
						}
					} else if (lowercase(subcommand) == "unload") {
						std::string modfile;
						tokens >> modfile;
						if (modfile == "module_diagnostics,so") {
							EmbedSimple("I suppose you think that's funny, dont you? *I'm sorry. can't do that, dave.*", msg.get_channel_id().get());
						} else {
							if (bot->Loader->Unload(modfile)) {
								EmbedSimple("Unloaded module: " + modfile, msg.get_channel_id().get());
							} else {
								EmbedSimple(std::string("Can't do that: ``") + bot->Loader->GetLastError() + "``", msg.get_channel_id().get());
							}
						}
					} else if (lowercase(subcommand) == "reload") {
						std::string modfile;
						tokens >> modfile;
						if (modfile == "module_diagnostics.so") {
							EmbedSimple("I suppose you think that's funny, dont you? *I'm sorry. can't do that, dave.*", msg.get_channel_id().get());
						} else {
							if (bot->Loader->Reload(modfile)) {
								EmbedSimple("Reloaded module: " + modfile, msg.get_channel_id().get());
							} else {
								EmbedSimple(std::string("Can't do that: ``") + bot->Loader->GetLastError() + "``", msg.get_channel_id().get());
							}
						}
					} else {
						EmbedSimple("Sudo **what**? I don't know what that command means.", msg.get_channel_id().get());
					}
				}
			} else {
				EmbedSimple("Make your own sandwich, mortal.", msg.get_channel_id().get());
			}

			return false;
		}
		return true;
	}
};

ENTRYPOINT(DiagnosticsModule);

