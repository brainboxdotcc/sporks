#pragma once

#include "bot.h"

class Module;
class ModuleLoader;

/** Implementation-specific flags which may be set in Module constructor by calling Attach()
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
	I_OnPresenceUpdate,
        I_END
};

/**
 * This #define allows us to call a method in all loaded modules in a readable simple way, e.g.:
 * 'FOREACH_MOD(I_OnGuildAdd,OnGuildAdd(guildinfo));'
 * NOTE: Locks mutex - two FOREACH_MOD() can't run asyncronously in two different threads. This is
 * to prevent one thread loading/unloading a module, changing the arrays/vectors while another thread
 * is running an event.
 */
#define FOREACH_MOD(y,x) { \
	std::lock_guard<std::mutex> lock(Loader->mtx); \
	Loader->ClearEvent(); \
        for (auto _i = Loader->EventHandlers[y].begin(); _i != Loader->EventHandlers[y].end(); ++_i) \
        { \
                try \
                { \
                        if (!(*_i)->x) { \
				Loader->ClaimEvent(); \
				break; \
			} \
                } \
                catch (std::exception& modexcept) \
                { \
                        core.log->error("Exception caught in module: {}", modexcept.what()); \
			Loader->ClaimEvent(); \
                } \
        } \
};


/** Defines the signature of the module's entrypoint function */
typedef Module* (initfunctype) (Bot*, ModuleLoader*);

/** A map representing the list of modules as external systems see it */
typedef std::map<std::string, Module*> ModMap;

/**
 * ModuleNative contains the OS level details of the module, e.g. the handle returned by dlopen()
 * and the last error message string, also a pointer to the init_module() function within the module.
 */
struct ModuleNative {
	initfunctype* init;
	void* dlopen_handle;
	const char *err;
	Module* module_object;
};

/**
 * ModuleLoader handles loading and unloading of modules at runtime, and maintains a list of loaded
 * modules. It can be queried for this list.
 */
class ModuleLoader {
	Bot* bot;

	/* A map of ModuleNatives used to manage the loaded module list */
	std::map<std::string, ModuleNative> Modules;

	/* Retrieve a named symbol from a shared object file */
	bool GetSymbol(ModuleNative &native, const char *sym_name);

	/* The module list as external systems see it, a map of string filenames to
	 * module objects
	 */
	ModMap ModuleList;

	bool claimed;

	std::string lasterror;

public:
	/* Module loader mutex */
	std::mutex mtx;

	/* An array of vectors indicating which modules are watching which events */
	std::vector<Module*> EventHandlers[I_END];

	ModuleLoader(Bot* creator);
	virtual ~ModuleLoader();

	/* Attach a module to an event. Only events a module explicitly attaches to will be
	 * called for that module, this allows a module to turn events on and off as it needs
	 * on the fly.
	 */
	void Attach(const std::vector<Implementation> &i, Module* mod);

	/* Detach a module from an event, opposite of Attach()
	 */
	void Detach(const std::vector<Implementation> &i, Module* mod);

	/* Load a module from a shared object file. The path is relative to the bot's executable.
	 */
	bool Load(const std::string &filename);

	/* Unload a module from memory. Calls the Module class's destructor and then dlclose().
	 */
	bool Unload(const std::string &filename);

	/* Unload and then Load a module
	 */
	bool Reload(const std::string &filename);

	/* Enumerate modules within modules.json and load them all. Any modules that are
	 * already loaded will be ignored, so we can use this to load new modules on rehash
	 */
	void LoadAll();

	/* Get a list of all loaded modules */
	const ModMap& GetModuleList() const;

	/* Claim an event so that the core stops processing it */
	void ClaimEvent();

	/* Cler an event so that the core can continue processing it */
	void ClearEvent();

	/* Returns true if the last event was claimed */
	bool IsEventClaimed();

	/** Returns last error message from Load(), or empty string */
	const std::string& GetLastError();
};

/**
 * All modules that can be loaded at runtime derive from the Module class. This class contains
 * virtual methods for each event triggered by the aegis library that the bot handles internally,
 * plus some helper methods to identify and report the version number of the module.
 */
class Module {
protected:
	/* Pointer back to the bot instance that created this module */
	Bot* bot;
public:
	/* Instantiate a module, the ModuleLoader is passed in so that we can attach events */
	Module(Bot* instigator, ModuleLoader* ml);

	virtual ~Module();

	/* Report version and description of module */
	virtual std::string GetVersion();
	virtual std::string GetDescription();

	/* Aegis events */
	virtual bool OnChannelCreate(const aegis::gateway::events::channel_create &channel);
	virtual bool OnReady(const aegis::gateway::events::ready &ready);
	virtual bool OnChannelDelete(const aegis::gateway::events::channel_delete &channel);
	virtual bool OnGuildCreate(const aegis::gateway::events::guild_create &guild);
	virtual bool OnGuildDelete(const aegis::gateway::events::guild_delete &guild);
	virtual bool OnGuildMemberAdd(const aegis::gateway::events::guild_member_add &gma);
	virtual bool OnMessage(const aegis::gateway::events::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions);
	virtual bool OnPresenceUpdate();

	/* Emit a simple text only embed to a channel, many modules use this for error reporting */
	void EmbedSimple(const std::string &message, int64_t channelID);
};


/* A macro that lets us simply define the entrypoint of a module by name */
#define ENTRYPOINT(mod_class_name) extern "C" Module* init_module(Bot* instigator, ModuleLoader* ml) { return new mod_class_name(instigator, ml); }

