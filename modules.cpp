#include "modules.h"
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <link.h>
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
	ModuleNative m;

	m.err = nullptr;
	m.dlopen_handle = nullptr;
	m.module_object = nullptr;
	m.init = nullptr;

	bot->core.log->info("Loading module \"{}\"", filename);

	if (Modules.find(filename) == Modules.end() || Modules[filename].err) {

		char buffer[PATH_MAX + 1];
		getcwd(buffer, PATH_MAX);
		std::string full_module_spec = std::string(buffer) + "/" + filename;

		m.dlopen_handle = dlopen(full_module_spec.c_str(), RTLD_NOW | RTLD_LOCAL);
		if (!m.dlopen_handle) {
			m.err = dlerror();
			Modules[filename] = m;
			lasterror = m.err;
			bot->core.log->error("Can't load module: {}", m.err);
			return false;
		} else {
			if (!GetSymbol(m, "init_module")) {
				bot->core.log->error("Can't load module: {}", m.err ? m.err : "General error");
				Modules[filename] = m;
				lasterror = m.err ? m.err : "General error";
				return false;
			} else {
				bot->core.log->debug("Module {} loaded", filename);
				m.module_object = m.init(bot, this);
				/* In the event of a missing module_init symbol, dlsym() returns a valid pointer to a function that returns -1 as its pointer. Why? I don't know.
				 * FIXME find out why.
				*/
				if (!m.module_object || (uint64_t)m.module_object == 0xffffffffffffffff) {
					bot->core.log->error("Can't load module: Invalid module pointer returned. No symbol?");
					lasterror = "Not a Sporks module (symbol init_module not found)";
					Modules[filename] = m;
					return false;
				}
				bot->core.log->debug("Module {} initialised", filename);
			}
		}
		Modules[filename] = m;
		ModuleList[filename] = m.module_object;
		lasterror = "";
		return true;
	}
	return false;
}

const std::string& ModuleLoader::GetLastError()
{
	return lasterror;
}

bool ModuleLoader::Unload(const std::string &filename)
{
	auto m = Modules.find(filename);

	if (m == Modules.end()) {
		lasterror = "Module is not loaded";
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
		//printf("dlopen_handle=0x%016x, native.init=0x%016x native.err=\"%s\" dlsym=0x%016x sym_name=%s\n", native.dlopen_handle, native.init, native.err ? native.err : "<NULL>", dlsym(native.dlopen_handle, sym_name), sym_name);
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

bool Module::OnMessage(const aegis::gateway::events::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions)
{
	return true;
}

void Module::EmbedSimple(const std::string &message, int64_t channelID)
{
	std::stringstream s;
	json embed_json;

	s << "{\"color\":16767488, \"description\": \"" << message << "\"}";

	try {
		embed_json = json::parse(s.str());
	}
	catch (const std::exception &e) {
		bot->core.log->error("Invalid json for channel {} created by EmbedSimple: ", channelID, s.str());
	}
	aegis::channel* channel = bot->core.find_channel(channelID);
	if (channel) {
		channel->create_message_embed("", embed_json);
	} else {
		bot->core.log->error("Invalid channel {} passed to EmbedSimple", channelID);
	}
}
