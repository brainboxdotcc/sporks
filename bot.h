#pragma once

#include "sleepy_discord/sleepy_discord.h"
#include <vector>
#include <string>
#include <unordered_map>

typedef std::unordered_map<std::string, SleepyDiscord::Channel> ChannelCache;

class Bot : public SleepyDiscord::DiscordClient {
	
	class PCRE* helpmessage;
	class PCRE* configmessage;
public:
	std::vector<SleepyDiscord::Server> serverList;
	ChannelCache channelList;
	SleepyDiscord::User user;

	using SleepyDiscord::DiscordClient::DiscordClient;

	void setup();

	void onReady(SleepyDiscord::Ready ready) override;
	void onServer(SleepyDiscord::Server server) override;
	void onChannel(SleepyDiscord::Channel channel) override;
	void onMessage(SleepyDiscord::Message message) override;
};

