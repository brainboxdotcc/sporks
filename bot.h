#pragma once

#include "sleepy_discord/sleepy_discord.h"
#include <vector>
#include <string>
#include <unordered_map>

typedef std::unordered_map<std::string, SleepyDiscord::Channel> ChannelCache;
typedef std::unordered_map<std::string, SleepyDiscord::User> UserCache;
typedef std::unordered_map<std::string, std::vector<std::string>> RandomNickCache;

class Bot : public SleepyDiscord::DiscordClient {
	
	class PCRE* helpmessage;
	class PCRE* configmessage;
public:
	std::vector<SleepyDiscord::Server> serverList;
	ChannelCache channelList;
	UserCache userList;
	RandomNickCache nickList;
	SleepyDiscord::User user;

	using SleepyDiscord::DiscordClient::DiscordClient;

	void setup();

	void onReady(SleepyDiscord::Ready ready) override;
	void onServer(SleepyDiscord::Server server) override;
	void onMember(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, SleepyDiscord::ServerMember member) override;
	void onChannel(SleepyDiscord::Channel channel) override;
	void onMessage(SleepyDiscord::Message message) override;
};

