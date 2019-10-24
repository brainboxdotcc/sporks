#pragma once

#include "sleepy_discord/sleepy_discord.h"
#include <pcre.h>
#include <vector>

class Bot : public SleepyDiscord::DiscordClient {
	
	pcre* message_match;
public:
	std::vector<SleepyDiscord::Server> serverList;

	using SleepyDiscord::DiscordClient::DiscordClient;

	void setup();

	void onServer(SleepyDiscord::Server server) override;

	void onChannel(SleepyDiscord::Channel channel) override;

	void onMessage(SleepyDiscord::Message message) override;
};

