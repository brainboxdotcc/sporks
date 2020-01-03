/************************************************************************************
 * 
 * Sporks, the learning, scriptable Discord bot!
 *
 * Copyright 2019 Craig Edwards <support@sporks.gg>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/

#include <sporks/modules.h>
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <link.h>
#include <dlfcn.h>
#include <sstream>

/**
 * String versions of the enum Implementation values, for display only
 */
const char* StringNames[I_END + 1] = {
	"I_BEGIN",
	"I_OnMessage",
	"I_OnReady",
	"I_OnChannelCreate",
	"I_OnChannelDelete",
	"I_OnGuildMemberAdd",
	"I_OnGuildCreate",
	"I_OnGuildDelete",
	"I_OnPresenceUpdate",
	"I_OnRestEnd",
	"I_END"
};

ModuleLoader::ModuleLoader(Bot* creator) : bot(creator)
{
	bot->core.log->info("Module loader initialising...");
}

ModuleLoader::~ModuleLoader()
{
}

/**
 * Attach an event to a module. Rather than just calling all events at all times, an event can be enabled or
 * disabled with Attach() and Detach(), this allows a module to programatically turn events on and off for itself.
 */
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

/**
 * Detach an event from a module, oppsite of Attach() above.
 */
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

/**
 * Set event as unclaimed by a module
 */
void ModuleLoader::ClearEvent()
{
	claimed = false;
}

/**
 * Set event as claimed by a module
 */
void ModuleLoader::ClaimEvent()
{
	claimed = true;
}

/**
 * Returns true if the last event was claimed by a module
 */
bool ModuleLoader::IsEventClaimed()
{
	return claimed;
}

/**
 * Return a reference to the module list
 */
const ModMap& ModuleLoader::GetModuleList() const
{
	return ModuleList;
}

/**
 * Load a module. Returns false on failure, true on success.
 * Sets the error message returned by GetLastError().
 */
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

/**
 * Returns the last error caused by Load() or Unload(), or an empty string for no error.
 */
const std::string& ModuleLoader::GetLastError()
{
	return lasterror;
}

/**
 * Unload a module from memory. Returns true on success or false on failure.
 */
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

/**
 * Unload, then reload a loaded module. Returns true on success or false on failure.
 * Failure to unload causes load to be skipped, so you can't use this function to load
 * a module that isnt loaded.
 */
bool ModuleLoader::Reload(const std::string &filename)
{
	/* Short-circuit evaluation here means that if Unload() returns false,
	 * Load() won't be called at all.
	 */
	return (Unload(filename) && Load(filename));
}

/**
 * Load all modules from the config file modules.json
 */
void ModuleLoader::LoadAll()
{
	json document;
	std::ifstream configfile("../config.json");
	configfile >> document;
	json modlist = document["modules"];
	for (auto entry = modlist.begin(); entry != modlist.end(); ++entry) {
		std::string modulename = entry->get<std::string>();
		this->Load(modulename);
	}
}

/**
 * Return a given symbol name from a shared object represented by the ModuleNative value.
 */
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

bool Module::OnPresenceUpdate()
{
	return true;
}

bool Module::OnRestEnd(std::chrono::steady_clock::time_point start_time, uint16_t code)
{
	return true;
}

/**
 * Output a simple embed to a channel consisting just of a message.
 */
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
