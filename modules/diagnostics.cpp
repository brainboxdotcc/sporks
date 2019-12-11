#include "../modules.h"

class DiagnosticsModule : public Module {
public:
	DiagnosticsModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml)
	{
		Implementation eventlist[] = { I_OnMessage };
		ml->Attach(eventlist, this, sizeof(eventlist) / sizeof(Implementation));
	}

	virtual std::string GetVersion()
	{
		return "1.0";
	}
	virtual std::string GetDescription()
	{
		return "Diagnostic Commands";
	}

	virtual bool OnMessage(const aegis::gateway::events::message_create &message)
	{
		return true;
	}
};

MODULE_INIT(DiagnosticsModule);

