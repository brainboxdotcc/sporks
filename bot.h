/*************************************************************************************
 *
 * Sporks Discord Connector, version 3
 * 
 * Sporks itself is a botnix instance, running the infobot and telnet modules. Rather
 * than rewrite this complex bit of code, we connect to it via this scalable connector,
 * which issues it commands over its telnet port. All the commands are queued, to prevent
 * swamping botnix with telnet connections and commands (like the PHP version has potential
 * to do by accident!) and outputs similarly queued to prevent flooding discord.
 *
 * There are two caching mechanisms for discord data, first is a set of unordered maps,
 * within this class, which hold servers, channels and users by Snowflake ID. The
 * second level is a mysql databse, which exists to feed information to the dashboard
 * website and give some persistence to the data. Note that from the bot's perspective
 * this data is write-only and the bot will only ever look at its unordered_maps, and
 * from the website's perspective this data is read-only and it will look to the discord
 * API for authentication and current server list of a user.
 *
 *************************************************************************************/
#pragma once

#include <aegis.hpp>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "queue.h"

using json = nlohmann::json;

/*typedef std::unordered_map<std::string, SleepyDiscord::Channel> ChannelCache;
typedef std::unordered_map<std::string, SleepyDiscord::User> UserCache;
typedef std::unordered_map<std::string, std::vector<std::string>> RandomNickCache;
typedef std::unordered_map<std::string, SleepyDiscord::Server> BotServerCache;*/

struct QueueStats {
	size_t inputs;
	size_t outputs;
	size_t users;
};

class Bot {

	/* True if bot is running in development mode */
	bool dev;
	
	/* Regexes for matching help and config commands */
	class PCRE* helpmessage;
	class PCRE* configmessage;

	/* Threads */
	std::thread* thr_input;
	std::thread* thr_output;
	std::thread* thr_userqueue;
	std::thread* thr_presence;

	/* Thread safety for caches and queues */
	std::mutex input_mutex;
	std::mutex output_mutex;
	std::mutex channel_hash_mutex;
	std::mutex user_cache_mutex;

	/* Input and output queue, lists of messages awaiting processing, or to be sent to channels */
	Queue inputs;
	Queue outputs;

	/* Set to true if all threads are to end */
	bool terminate;

	/* Join and delete a non-null pointer to std::thread */
	void DisposeThread(std::thread* thread);

	/* Thread handlers */
	void InputThread();		/* Processes input lines from channel messages, complex responses can take upwards of 250ms */
	void OutputThread();		/* Outputs lines due to be sent to channel messages, after being processed by the input thread */
	void SaveCachedUsersThread();	/* If there are any users in the userqueue, this thread updates/inserts them on a mysql table in the background */
	void UpdatePresenceThread();	/* Updates the bot presence every 120 seconds */

public:
	/* Aegis core */
	aegis::core &core;

	/* Caches, unordered map storage of discord's data (sleepy_discord doesn't cache much
	 * at all, so we must ensure we maintain our own caches)
	 */
	//BotServerCache serverList;
	//ChannelCache channelList;
	//UserCache userList;
	//RandomNickCache nickList;	/* Special case, contains a vector of nicknames per-server for selecting a random nickname only */

	/* Userqueue: a queue of users waiting to be written to SQL for the dashboard */
	//std::queue<SleepyDiscord::User> userqueue;

	/* The bot's user details from ready event */
	aegis::gateway::objects::user user;

	/* Shard details, filled by ctor */
	uint32_t ShardID;
	uint32_t MaxShards;

	Bot(uint32_t shard_id, uint32_t max_shards, bool development, aegis::core &aegiscore);
	virtual ~Bot();

	QueueStats GetQueueStats();

	/* Shorthand to get bot's user id */
	int64_t getID();

	void onReady(aegis::gateway::events::ready ready);

	/* Caches server entry, iterates and caches channels, iterates and caches users, queues users to be stored in SQL */
	//void onServer(const SleepyDiscord::Server &server) override;

	/* Caches user, also stores user in SQL */
	//void onMember(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, SleepyDiscord::ServerMember member) override;

	/* Caches channel, also creates channel settings row in SQL if needed */
	//void onChannel(const SleepyDiscord::Channel &channel) override;

	/* Passes incoming messages to the input queue, and directly handles commands */
	void onMessage(aegis::gateway::events::message_create message);

	/* Theres some weird allocator crap going off with rapidjson as it's used in sleepy_discord,
	 * so this is static to prevent a compilation error (!)
	 */
	static std::string GetConfig(const std::string &name);
};

