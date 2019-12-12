#include "modules.h"
#include <dlfcn.h>
#include <sstream>

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

ModuleLoader::~ModuleLoader()
{
}

void ModuleLoader::Attach(const std::vector<Implementation> &i, Module* mod)
{
	for (auto n = i.begin(); n != i.end(); ++n) {
		if (std::find(EventHandlers[*n].begin(), EventHandlers[*n].end(), mod) == EventHandlers[*n].end()) {
			EventHandlers[*n].push_back(mod);
			bot->core.log->debug("Module \"{}\" attached event \"{}\"", mod->GetDescription(), StringNames[*n]);
		} else {
			bot->core.log->warn("Module \"{}\" is already attached to event \"{}\"", mod->GetDescription(), StringNames[*n]);
		}
	}
}

void ModuleLoader::Detach(const std::vector<Implementation> &i, Module* mod)
{
	for (auto n = i.begin(); n != i.end(); ++n) {
		auto it = std::find(EventHandlers[*n].begin(), EventHandlers[*n].end(), mod);
		if (it != EventHandlers[*n].end()) {
			EventHandlers[*n].erase(it);
			bot->core.log->debug("Module \"{}\" detached event \"{}\"", mod->GetDescription(), StringNames[*n]);
		}
	}
}

void ModuleLoader::ClearEvent()
{
	claimed = false;
}

void ModuleLoader::ClaimEvent()
{
	claimed = true;
}

bool ModuleLoader::IsEventClaimed()
{
	return claimed;
}

const ModMap& ModuleLoader::GetModuleList() const
{
	return ModuleList;
}

bool ModuleLoader::Load(const std::string &filename)
{
	std::lock_guard<std::mutex> lock(mtx);

	ModuleNative m;
	m.err = nullptr;
	m.dlopen_handle = nullptr;

	bot->core.log->info("Loading module \"{}\"", filename);

	if (Modules.find(filename) == Modules.end() || Modules[filename].err) {

		char buffer[PATH_MAX + 1];
		getcwd(buffer, PATH_MAX);
		std::string full_module_spec = std::string(buffer) + "/" + filename;

		m.dlopen_handle = dlopen(full_module_spec.c_str(), RTLD_NOW | RTLD_LOCAL);
		if (!m.dlopen_handle) {
			m.err = dlerror();
			Modules[filename] = m;
			bot->core.log->error("Can't load module: {}", m.err);
			return false;
		} else {
			if (!GetSymbol(m, "init_module")) {
				bot->core.log->error("Can't load module: {}", m.err ? m.err : "General error");
				Modules[filename] = m;
				return false;
			} else {
				bot->core.log->debug("Module {} loaded", filename);
				m.module_object = m.init(bot, this);
				if (!m.module_object) {
					bot->core.log->error("Can't load module: Invalid module pointer returned");
					Modules[filename] = m;
					return false;
				}
				bot->core.log->debug("Module {} initialised", filename);
			}
		}
		Modules[filename] = m;
		ModuleList[filename] = m.module_object;
		return true;
	}
	return false;
}

bool ModuleLoader::Unload(const std::string &filename)
{
	std::lock_guard<std::mutex> lock(mtx);

	auto m = Modules.find(filename);

	if (m == Modules.end()) {
		return false;
	}

	ModuleNative mod = m->second;

	/* Remove attached events */
	for (int j = I_BEGIN; j != I_END; ++j) {
		std::remove(EventHandlers[j].begin(), EventHandlers[j].end(), mod.module_object);
	}
	/* Remove module entry */
	Modules.erase(m);

	auto v = ModuleList.find(filename);
	if (v != ModuleList.end()) {
		ModuleList.erase(v);
	}

	if (mod.module_object) {
		delete mod.module_object;
	}

	/* Remove module from memory */
	if (mod.dlopen_handle) {
		dlclose(mod.dlopen_handle);
	}

	return true;
}

bool ModuleLoader::Reload(const std::string &filename)
{
	return (Unload(filename) && Load(filename));
}

void ModuleLoader::LoadAll()
{
	json document;
	std::ifstream configfile("../modules.json");
	configfile >> document;
	for (auto entry = document.begin(); entry != document.end(); ++entry) {
		std::string modulename = entry->get<std::string>();
		this->Load(modulename);
	}
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
	} else {
		native.err = "ModuleLoader::GetSymbol(): Invalid dlopen() handle";
		return false;
	}
	return true;
}

Module::Module(Bot* instigator, ModuleLoader* ml) : bot(instigator)
{
}

Module::~Module()
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

bool Module::OnMessage(const aegis::gateway::events::message_create &message, const std::string& clean_message, bool mentioned)
{
	return true;
}

