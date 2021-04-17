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
 * Sporks itself is a botnix instance, running the infobot and telnet modules. Rather
 * than rewrite this complex bit of code, we connect to it via this scalable connector,
 * which issues it commands over its telnet port. All the commands are queued, to prevent
 * swamping botnix with telnet connections and commands (like the PHP version has potential
 * to do by accident!) and outputs similarly queued to prevent flooding discord.
 *
 * All major parts of this bot are modular, and can be hot-reloaded on the fly to prevent
 * having to restart the shards. Please see the modules directory for source code.
 *
 ************************************************************************************/

#pragma once

#include <dpp/dpp.h>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>

using json = nlohmann::json;

class Module;
class ModuleLoader;

class Bot {

	/* True if bot is running in development mode */
	bool dev;

	/* True if bot is running in testing mode */
	bool test;

	/* True if the bot has member intents */
	bool memberintents;

	/* Threads */
	std::thread* thr_presence;

	/* Set to true if all threads are to end */
	bool terminate;

	uint32_t shard_init_count;

	/* Thread handlers */
	void UpdatePresenceThread();	/* Updates the bot presence every 120 seconds */

	void SetSignals();

public:
	/* Aegis core */
	class dpp::cluster* core;

	/* Generic named counters */
	std::map<std::string, uint64_t> counters;

	/* The bot's user details from ready event */
	dpp::user user;

	uint64_t sent_messages;
	uint64_t received_messages;

	Bot(bool development, bool testing, bool intents, dpp::cluster* dppcluster);
	virtual ~Bot();

	bool IsDevMode();
	bool IsTestMode();
	bool HasMemberIntents();

	ModuleLoader* Loader;

	/* Join and delete a non-null pointer to std::thread */
	void DisposeThread(std::thread* thread);

	/* Shorthand to get bot's user id */
	int64_t getID();

	void onReady(const dpp::ready_t &ready);
	void onServer(const dpp::guild_create_t &gc);
	void onMember(const dpp::guild_member_add_t &gma);
	void onChannel(const dpp::channel_create_t &channel);
	void onMessage(const dpp::message_create_t &message);
	void onChannelDelete(const dpp::channel_delete_t &cd);
	void onServerDelete(const dpp::guild_delete_t &gd);
	void onTypingStart (const dpp::typing_start_t &event);
	void onMessageUpdate (const dpp::message_update_t &event);
	void onMessageDelete (const dpp::message_delete_t &event);
	void onMessageDeleteBulk (const dpp::message_delete_bulk_t &event);
	void onGuildUpdate (const dpp::guild_update_t &event);
	void onMessageReactionAdd (const dpp::message_reaction_add_t &event);
	void onMessageReactionRemove (const dpp::message_reaction_remove_t &event);
	void onMessageReactionRemoveAll (const dpp::message_reaction_remove_all_t &event);
	void onUserUpdate (const dpp::user_update_t &event);
	void onResumed (const dpp::resumed_t &event);
	void onChannelUpdate (const dpp::channel_update_t &event);
	void onChannelPinsUpdate (const dpp::channel_pins_update_t &event);
	void onGuildBanAdd (const dpp::guild_ban_add_t &event);
	void onGuildBanRemove (const dpp::guild_ban_remove_t &event);
	void onGuildEmojisUpdate (const dpp::guild_emojis_update_t &event);
	void onGuildIntegrationsUpdate (const dpp::guild_integrations_update_t &event);
	void onGuildMemberRemove (const dpp::guild_member_remove_t &event);
	void onGuildMemberUpdate (const dpp::guild_member_update_t &event);
	void onGuildMembersChunk (const dpp::guild_members_chunk_t &event);
	void onGuildRoleCreate (const dpp::guild_role_create_t &event);
	void onGuildRoleUpdate (const dpp::guild_role_update_t &event);
	void onGuildRoleDelete (const dpp::guild_role_delete_t &event);
	void onPresenceUpdate (const dpp::presence_update_t &event);
	void onVoiceStateUpdate (const dpp::voice_state_update_t &event);
	void onVoiceServerUpdate (const dpp::voice_server_update_t &event);
	void onWebhooksUpdate (const dpp::webhooks_update_t &event);

	static std::string GetConfig(const std::string &name);

	static void SetSignal(int signal);
};
