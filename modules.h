#pragma once

#include "bot.h"

class Module;
class ModuleLoader;

/** Priority types which can be returned from Module::Prioritize()
 */
enum Priority { PRIORITY_FIRST, PRIORITY_DONTCARE, PRIORITY_LAST };

/** Implementation-specific flags which may be set in Module::Implements()
 */
enum Implementation
{
        I_BEGIN,
	I_OnMessage,
	I_OnReady,
	I_OnChannelCreate,
	I_OnChannelDelete,
	I_OnGuildMemberAdd,
	I_OnGuildCreate,
	I_OnGuildDelete,
        I_END
};

/**
 * This #define allows us to call a method in all
 * loaded modules in a readable simple way, e.g.:
 * 'FOREACH_MOD(I_OnGuildAdd,OnGuildAdd(guildinfo));'
 */
#define FOREACH_MOD(y,x) do { \
        for (auto _i = Loader->EventHandlers[y].begin(); _i != Loader->EventHandlers[y].end(); ++_i) \
        { \
                try \
                { \
                        if (!(*_i)->x) break; \
                } \
                catch (std::exception& modexcept) \
                { \
                        core.log->error("Exception caught in module: {}",modexcept.what()); \
                } \
        } \
} while (0);


typedef Module* (initfunctype) (Bot*, ModuleLoader*);

struct ModuleNative {
	initfunctype* init;
	void* dlopen_handle;
	const char *err;
	Module* module_object;
};

class ModuleLoader {
	Bot* bot;

	std::map<std::string, ModuleNative> Modules;
	bool GetSymbol(ModuleNative &native, const char *sym_name);

public:
	std::vector<Module*> EventHandlers[I_END];

	ModuleLoader(Bot* creator);
	virtual ~ModuleLoader();
	void Attach(Implementation* i, Module* mod, size_t sz);
	bool Load(const std::string &filename);
	bool Unload(const std::string &filename);
	bool Reload(const std::string &filename);
	void LoadAll();
};

class Module {
protected:
	Bot* bot;
public:
	Module(Bot* instigator, ModuleLoader* ml);
	virtual ~Module();
	virtual std::string GetVersion();
	virtual std::string GetDescription();
	virtual bool OnChannelCreate(const aegis::gateway::events::channel_create &channel);
	virtual bool OnReady(const aegis::gateway::events::ready &ready);
	virtual bool OnChannelDelete(const aegis::gateway::events::channel_delete &channel);
	virtual bool OnGuildCreate(const aegis::gateway::events::guild_create &guild);
	virtual bool OnGuildDelete(const aegis::gateway::events::guild_delete &guild);
	virtual bool OnGuildMemberAdd(const aegis::gateway::events::guild_member_add &gma);
	virtual bool OnMessage(const aegis::gateway::events::message_create &message);
};


#define MODULE_INIT(y) extern "C" Module* init_module(Bot* instigator, ModuleLoader* ml) { return new y(instigator, ml); }

