#include "../modules.h"

/**
 * Provides diagnostic commands for monitoring the bot and debugging it interactively while it's running.
 */

class DiagnosticsModule : public Module
{
public:
	DiagnosticsModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml)
	{
		Implementation eventlist[] = { I_OnMessage };
		ml->Attach(eventlist, this, sizeof(eventlist) / sizeof(Implementation));
	}

	virtual ~DiagnosticsModule()
	{
	}

	virtual std::string GetVersion()
	{
		return "1.0";
	}

	virtual std::string GetDescription()
	{
		return "module_diagnostics.so - Diagnostic Commands";
	}

	virtual bool OnMessage(const aegis::gateway::events::message_create &message)
	{
		bot->core.log->debug("Diagnostics OnMessage: {}", message.msg.get_content());
		return true;
	}
};

MODULE_INIT(DiagnosticsModule);

