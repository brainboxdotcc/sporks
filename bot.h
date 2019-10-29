#pragma once

#include "sleepy_discord/sleepy_discord.h"
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "queue.h"

typedef std::unordered_map<std::string, SleepyDiscord::Channel> ChannelCache;
typedef std::unordered_map<std::string, SleepyDiscord::User> UserCache;
typedef std::unordered_map<std::string, std::vector<std::string>> RandomNickCache;
typedef std::unordered_map<std::string, SleepyDiscord::Server> BotServerCache;

struct QueueStats {
	size_t inputs;
	size_t outputs;
	size_t users;
};

class Bot : public SleepyDiscord::DiscordClient {
	
	class PCRE* helpmessage;
	class PCRE* configmessage;

	std::thread* thr_input;
	std::thread* thr_output;

	bool terminate;

public:
	BotServerCache serverList;
	ChannelCache channelList;
	UserCache userList;
	RandomNickCache nickList;
	SleepyDiscord::User user;
	uint32_t ShardID;
	uint32_t MaxShards;

	using SleepyDiscord::DiscordClient::DiscordClient;

	Bot(const std::string &token, uint32_t shard_id, uint32_t max_shards);
	virtual ~Bot();
	QueueStats GetQueueStats();
	void onReady(const SleepyDiscord::Ready &ready) override;
	void onServer(const SleepyDiscord::Server &server) override;
	void onMember(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, SleepyDiscord::ServerMember member) override;
	void onChannel(const SleepyDiscord::Channel &channel) override;
	void onMessage(const SleepyDiscord::Message &message) override;

	static std::string GetConfig(const std::string &name);

	void InputThread(std::mutex *input_mutex, std::mutex *output_mutex, std::mutex *channel_hash_mutex, Queue *inputs, Queue *outputs);
	void OutputThread(std::mutex* output_mutex, std::mutex* channel_hash_mutex, Queue* outputs);
};

