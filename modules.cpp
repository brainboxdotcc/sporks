#include "modules.h"
#include <dlfcn.h>

const char* StringNames[I_END + 1] = {
	"I_BEGIN",
	"I_OnMessage",
	"I_OnReady",
	"I_OnChannelCreate",
	"I_OnChannelDelete",
	"I_OnGuildMemberAdd",
	"I_OnGuildCreate",
	"I_OnGuildDelete",
	"I_END"
};

ModuleLoader::ModuleLoader(Bot* creator) : bot(creator)
{
	bot->core.log->info("Module loader initialising...");
}

void ModuleLoader::Attach(Implementation* i, Module* mod, size_t sz)
{
	for (size_t n = 0; n < sz; ++n) {
		if (std::find(EventHandlers[i[n]].begin(), EventHandlers[i[n]].end(), mod) == EventHandlers[i[n]].end()) {
			EventHandlers[i[n]].push_back(mod);
			bot->core.log->debug("Module \"{}\" attached event \"{}\"", mod->GetDescription(), StringNames[i[n]]);
		} else {
			bot->core.log->warn("Module \"{}\" is already attached to event \"{}\"", mod->GetDescription(), StringNames[i[n]]);
		}
	}
}

bool ModuleLoader::Load(const std::string &filename)
{
	ModuleNative m;
	m.err = nullptr;
	m.dlopen_handle = nullptr;

	bot->core.log->info("Loading module \"{}\"", filename);

	if (Modules.find(filename) == Modules.end() || Modules[filename].err) {

		m.dlopen_handle = dlopen(filename.c_str(), RTLD_NOW|RTLD_LOCAL);
		if (!m.dlopen_handle) {
			m.err = dlerror();
			Modules[filename] = m;
			bot->core.log->error("Can't load module: {}", m.err);
			return false;
		} else {
			if (!GetSymbol(m, "init_module")) {
				m.err = "Unable to find init_module() entrypoint";
				bot->core.log->error("Can't load module: {}", m.err);
				Modules[filename] = m;
				return false;
			} else {
				bot->core.log->debug("Module {} loaded", filename);
				m.module_object = m.init(bot, this);
				bot->core.log->debug("Module {} initialised", filename);
			}
		}
		Modules[filename] = m;
		return true;
	}
	return false;
}

bool ModuleLoader::GetSymbol(ModuleNative &native, const char *sym_name)
{
	/* Find exported symbol in shared object */
	if (native.dlopen_handle) {
		dlerror(); // clear value
		native.init = (initfunctype*)dlsym(native.dlopen_handle, sym_name);
		native.err = dlerror();
		if (!native.init || native.err) {
			return false;
		}
	}
	return true;
}

Module::Module(Bot* instigator, ModuleLoader* ml) : bot(instigator)
{
}

std::string Module::GetVersion()
{
	return "";
}

std::string Module::GetDescription()
{
	return "";
}

bool Module::OnChannelCreate(const aegis::gateway::events::channel_create &channel)
{
	return true;
} 

bool Module::OnReady(const aegis::gateway::events::ready &ready)
{
	return true;
}

bool Module::OnChannelDelete(const aegis::gateway::events::channel_delete &channel)
{
	return true;
}

bool Module::OnGuildCreate(const aegis::gateway::events::guild_create &guild)
{
	return true;
}

bool Module::OnGuildDelete(const aegis::gateway::events::guild_delete &guild)
{
	return true;
}

bool Module::OnGuildMemberAdd(const aegis::gateway::events::guild_member_add &gma)
{
	return true;
}

bool Module::OnMessage(const aegis::gateway::events::message_create &message)
{
	return true;
}
