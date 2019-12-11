#pragma once

#include "bot.h"

class Module;

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
        auto safei; \
        for (auto _i = Modules->EventHandlers[y].begin(); _i != Modules->EventHandlers[y].end(); ) \
        { \
                safei = _i; \
                ++safei; \
                try \
                { \
                        (*_i)->x ; \
                } \
                catch (std::exception& modexcept) \
                { \
                        core.log->error("Exception caught in module: {}",modexcept.what()); \
                } \
                _i = safei; \
        } \
} while (0);

class ModuleLoader {
	Bot* bot;
public:
	ModuleLoader(Bot* creator);
	void Attach(Implementation* i, Module* mod, size_t sz);
	bool Load(const std::string &filename);
};

class Module {
	Bot* bot;
public:
	Module(Bot* instigator, ModuleLoader* ml);
	virtual std::string GetVersion();
	virtual std::string GetDescription();
	virtual void OnChannelCreate(const aegis::gateway::events::channel_create &channel);
	virtual void OnReady(const aegis::gateway::events::ready &ready);
	virtual void OnChannelDelete(const aegis::gateway::events::channel_delete &channel);
	virtual void OnGuildCreate(const aegis::gateway::events::guild_create &guild);
	virtual void OnGuildDelete(const aegis::gateway::events::guild_delete &guild);
	virtual void OnGuildMemberAdd(const aegis::gateway::events::guild_member_add &gma);
};
