#include "modules.h"
#include <dlfcn.h>

ModuleLoader::ModuleLoader(Bot* creator) : bot(creator)
{
}

void ModuleLoader::Attach(Implementation* i, Module* mod, size_t sz)
{
}


bool ModuleLoader::Load(const std::string &filename)
{
	ModuleNative m;
	m.err = nullptr;
	m.dlopen_handle = nullptr;

	if (Modules.find(filename) == Modules.end() || Modules[filename].err) {

		m.dlopen_handle = dlopen(filename.c_str(), RTLD_NOW|RTLD_LOCAL);
		if (!m.dlopen_handle) {
			m.err = dlerror();
			Modules[filename] = m;
			return false;
		} else {
			if (!GetSymbol(m, "init_module")) {
				m.err = "Unable to find init_module() entrypoint";
				Modules[filename] = m;
				return false;
			} else {
				m.module_object = m.init(bot, this);
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
