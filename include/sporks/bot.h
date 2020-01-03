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
 * There are two caching mechanisms for discord data, first is aegis itself, and the
 * second level is a mysql databse, which exists to feed information to the dashboard
 * website and give some persistence to the data. Note that from the bot's perspective
 * this data is write-only and the bot will only ever look at the data in aegis, and
 * from the website's perspective this data is read-only and it will look to the discord
 * API for authentication and current server list of a user.
 *
 * All major parts of this bot are modular, and can be hot-reloaded on the fly to prevent
 * having to restart the shards. Please see the modules directory for source code.
 *
 ************************************************************************************/

#pragma once

#include <aegis.hpp>
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
	
	/* Threads */
	std::thread* thr_userqueue;
	std::thread* thr_presence;

	/* Set to true if all threads are to end */
	bool terminate;

	/* Thread handlers */
	void SaveCachedUsersThread();	/* If there are any users in the userqueue, this thread updates/inserts them on a mysql table in the background */
	void UpdatePresenceThread();	/* Updates the bot presence every 120 seconds */

	void SetSignals();

public:

        /* Thread safety for caches and queues */
	std::mutex channel_hash_mutex;
	std::mutex user_cache_mutex;

	/* Aegis core */
	aegis::core &core;

	/* Userqueue: a queue of users waiting to be written to SQL for the dashboard */
	std::queue<aegis::gateway::objects::user> userqueue;

	/* The bot's user details from ready event */
	aegis::gateway::objects::user user;

	uint64_t sent_messages;
	uint64_t received_messages;

	Bot(bool development, aegis::core &aegiscore);
	virtual ~Bot();

	bool IsDevMode();

	ModuleLoader* Loader;

	/* Join and delete a non-null pointer to std::thread */
	void DisposeThread(std::thread* thread);

	/* Shorthand to get bot's user id */
	int64_t getID();

	void onReady(aegis::gateway::events::ready ready);

	/* Caches server entry, iterates and caches channels, iterates and caches users, queues users to be stored in SQL */
	void onServer(aegis::gateway::events::guild_create gc);

	/* Caches user, also stores user in SQL */
	void onMember(aegis::gateway::events::guild_member_add gma);

	/* Caches channel, also creates channel settings row in SQL if needed */
	void onChannel(aegis::gateway::events::channel_create channel);

	/* Passes incoming messages to the input queue, and directly handles commands */
	void onMessage(aegis::gateway::events::message_create message);

	/* Deletes channel settings from SQL database */
	void onChannelDelete(aegis::gateway::events::channel_delete cd);

	/* Deletes channel settings from SQL database for all channels on server */
	void onServerDelete(aegis::gateway::events::guild_delete gd);

	/* Returns details of time taken to execute a REST request */
	void onRestEnd(std::chrono::steady_clock::time_point start_time, uint16_t code);

	static std::string GetConfig(const std::string &name);

	static void SetSignal(int signal);
};

