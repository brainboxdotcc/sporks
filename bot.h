#pragma once

#include "sleepy_discord/sleepy_discord.h"
#include <pcre.h>
#include <vector>
#include <string>
#include <unordered_map>

typedef std::unordered_map<std::string, SleepyDiscord::Channel> ChannelCache;

class Bot : public SleepyDiscord::DiscordClient {
	
	pcre* message_match;
public:
	std::vector<SleepyDiscord::Server> serverList;
	ChannelCache channelList;

	using SleepyDiscord::DiscordClient::DiscordClient;

	void setup();

	void onServer(SleepyDiscord::Server server) override;

	void onChannel(SleepyDiscord::Channel channel) override;

	void onMessage(SleepyDiscord::Message message) override;
};

