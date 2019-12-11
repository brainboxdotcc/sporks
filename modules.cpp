#include "modules.h"

ModuleLoader::ModuleLoader(Bot* creator) : bot(creator)
{
}

void ModuleLoader::Attach(Implementation* i, Module* mod, size_t sz)
{
}


bool ModuleLoader::Load(const std::string &filename)
{
	return false;
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

void Module::OnChannelCreate(const aegis::gateway::events::channel_create &channel)
{
} 

void Module::OnReady(const aegis::gateway::events::ready &ready)
{
}

void Module::OnChannelDelete(const aegis::gateway::events::channel_delete &channel)
{
}

void Module::OnGuildCreate(const aegis::gateway::events::guild_create &guild)
{
}

void Module::OnGuildDelete(const aegis::gateway::events::guild_delete &guild)
{
}

void Module::OnGuildMemberAdd(const aegis::gateway::events::guild_member_add &gma)
{
}
