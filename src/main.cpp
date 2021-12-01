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
 ************************************************************************************/

#define SPDLOG_FMT_EXTERNAL
#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <sporks/bot.h>
#include <sporks/includes.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sporks/database.h>
#include <sporks/config.h>
#include <sporks/stringops.h>
#include <sporks/modules.h>

using json = nlohmann::json;

/**
 * Parsed configuration file
 */
json configdocument;

/**
 * Constructor (creates threads, loads all modules)
 */
Bot::Bot(bool development, bool testing, bool intents, dpp::cluster* dppcluster) : dev(development), test(testing), memberintents(intents), thr_presence(nullptr), terminate(false), shard_init_count(0), core(dppcluster), sent_messages(0), received_messages(0) {
	Loader = new ModuleLoader(this);
	Loader->LoadAll();

	thr_presence = new std::thread(&Bot::UpdatePresenceThread, this);
}

/**
 * Join and delete a thread
 */
void Bot::DisposeThread(std::thread* t) {
	if (t) {
		t->join();
		delete t;
	}

}

/**
 * Destructor
 */
Bot::~Bot() {
	terminate = true;

	DisposeThread(thr_presence);

	delete Loader;
}

/**
 * Returns the named string value from config.json
 */
std::string Bot::GetConfig(const std::string &name) {
	return configdocument[name].get<std::string>();
}

/**
 * Returns true if the bot is running in development mode (different token)
 */
bool Bot::IsDevMode() {
	return dev;
}

/**
 * Returns true if the bot is running in testing mode (live token, ignoring messages except on specific server)
 */
bool Bot::IsTestMode() {
	return test;
}

/** 
 * Returns true if the bot has member intents enabled, "GUILD_MEMBERS" which will eventually require discord HQ approval process.
 */
bool Bot::HasMemberIntents() {
	return memberintents;
}

/**
 * On adding a new server, the details of that server are inserted or updated in the shard map. We also make sure settings
 * exist for each channel on the server by calling getSettings() for each channel and throwing the result away, which causes
 * record creation. New users for the guild are pushed into the userqueue which is processed in a separate thread within
 * SaveCachedUsersThread().
 */
void Bot::onServer(const dpp::guild_create_t& gc) {
	FOREACH_MOD(I_OnGuildCreate, OnGuildCreate(gc));
}

/**
 * This runs its own thread that wakes up every 30 seconds (after an initial 2 minute warmup).
 * Modules can attach to it for a simple 30 second interval timer via the OnPresenceUpdate() method.
 */
void Bot::UpdatePresenceThread() {
	std::this_thread::sleep_for(std::chrono::seconds(120));
	while (!this->terminate) {
		FOREACH_MOD(I_OnPresenceUpdate, OnPresenceUpdate());
		std::this_thread::sleep_for(std::chrono::seconds(30));
	}
}

/**
 * Stores a new guild member to the database for use in the dashboard
 */
void Bot::onMember(const dpp::guild_member_add_t& gma) {
	FOREACH_MOD(I_OnGuildMemberAdd, OnGuildMemberAdd(gma));
}

/**
 * Returns the bot's snowflake id
 */
int64_t Bot::getID() {
	return this->user.id;
}

/**
 * Announces that the bot is online. Each shard receives one of the events.
 */
void Bot::onReady(const dpp::ready_t& ready) {
	this->user = core->me;
	FOREACH_MOD(I_OnReady, OnReady(ready));

	/* Event broadcast when all shards are ready */
	shard_init_count++;
	if (shard_init_count == core->get_shards().size()) {
		FOREACH_MOD(I_OnAllShardsReady, OnAllShardsReady());
	}
}

/**
 * Called on receipt of each message. We do our own cleanup of the message, sanitising any
 * mentions etc from the text before passing it along to modules. The bot's builtin ignore list
 * and a hard coded check against bots/webhooks and itself happen before any module calls,
 * and can't be overridden.
 */
void Bot::onMessage(const dpp::message_create_t &message) {

	if (!message.msg.author.id) {
		core->log(dpp::ll_info, fmt::format("Message dropped, no author: {}", message.msg.content));
		return;
	}
	/* Ignore self, and bots */
	if (message.msg.author.id != user.id && message.msg.author.is_bot() == false) {

		json settings;
		settings = getSettings(this, message.msg.channel_id, message.msg.guild_id);

		received_messages++;

		/* Ignore anyone on ignore list */
		std::vector<uint64_t> ignorelist = settings::GetIgnoreList(settings);
		if (message.msg.author.id && std::find(ignorelist.begin(), ignorelist.end(), message.msg.author.id) != ignorelist.end()) {
			core->log(dpp::ll_info, fmt::format("Message #{} dropped, user on channel ignore list", message.msg.id));
			return;
		}

		/* Replace all mentions with raw nicknames */
		bool mentioned = false;
		std::string mentions_removed = message.msg.content;
		std::vector<std::string> stringmentions;
		for (auto m = message.msg.mentions.begin(); m != message.msg.mentions.end(); ++m) {
			stringmentions.push_back(std::to_string(m->first.id));
			mentions_removed = ReplaceString(mentions_removed, std::string("<@") + std::to_string(m->first.id) + ">", m->first.username);
			mentions_removed = ReplaceString(mentions_removed, std::string("<@!") + std::to_string(m->first.id) + ">", m->first.username);
			if (m->first.id == user.id) {
				mentioned = true;
			}
		}

		std::string botusername = this->user.username;

		/* Remove bot's nickname from start of message, if it's there */
		while (mentions_removed.substr(0, botusername.length()) == botusername) {
			mentions_removed = trim(mentions_removed.substr(botusername.length(), mentions_removed.length()));
		}
		/* Remove linefeeds, they mess with botnix */
		mentions_removed = trim(mentions_removed);

		/* Call modules */
		FOREACH_MOD(I_OnMessage,OnMessage(message, mentions_removed, mentioned, stringmentions));
	}
}

void Bot::onChannel(const dpp::channel_create_t& channel_create) {
	FOREACH_MOD(I_OnChannelCreate, OnChannelCreate(channel_create));
}

void Bot::onChannelDelete(const dpp::channel_delete_t& cd) {
	FOREACH_MOD(I_OnChannelDelete, OnChannelDelete(cd));
}

void Bot::onServerDelete(const dpp::guild_delete_t& gd) {
	FOREACH_MOD(I_OnGuildDelete, OnGuildDelete(gd));
}

int main(int argc, char** argv) {

	int dev = 0;	/* Note: getopt expects ints, this is actually treated as bool */
	int test = 0;
	int members = 0;

	/* Set this specifically so that stringstreams don't do weird things on other locales printing decimal numbers for SQL */
	std::setlocale(LC_ALL, "en_GB.UTF-8");

	/* Parse command line parameters using getopt() */
	struct option longopts[] =
	{
		{ "dev",	no_argument,		&dev,		1 },
		{ "test",	no_argument,		&test,		1 },
		{ "members",	no_argument,		&members,	1 },
		{ 0, 0, 0, 0 }
	};

	/* These are our default intents for the bot, basically just receive messages, see reactions to the messages and see who's in our guilds */
	uint32_t intents = dpp::i_default_intents;

	/* Yes, getopt is ugly, but what you gonna do... */
	int index;
	char arg;

	/* opterr is an extern int. Doesn't smell of thread safety to me, bad GNU bad! */
	opterr = 0;
	while ((arg = getopt_long_only(argc, argv, "", longopts, &index)) != -1) {
		switch (arg) {
			case 0:
				/* getopt_long_only() set an int variable, just keep going */
			break;
			case '?':
			default:
				std::cerr << "Unknown parameter '" << argv[optind - 1] << "'\n";
				std::cerr << "Usage: " << argv[0] << " [-dev] [-test] [-members]\n\n";
				std::cerr << "-dev:     Run using development token\n";
				std::cerr << "-test:    Run using live token, but eat all outbound messages except on test server\n";
				std::cerr << "-members: Issue a GUILD_MEMBERS intent on shard registration\n";
				exit(1);
			break;
		}
	}

	/* This will eventually need approval from discord HQ, so make sure it's a command line parameter we have to explicitly enable */
	if (members) {
		intents |= dpp::i_guild_members;
	}

	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	/* Get the correct token from config file for either development or production environment */
	std::string token = (dev ? Bot::GetConfig("devtoken") : Bot::GetConfig("livetoken"));

	/* Connect to SQL database */
	if (!db::connect(Bot::GetConfig("dbhost"), Bot::GetConfig("dbuser"), Bot::GetConfig("dbpass"), Bot::GetConfig("dbname"), from_string<uint32_t>(Bot::GetConfig("dbport"), std::dec))) {
		std::cerr << "Database connection failed\n";
		exit(2);
	}

	/* It's go time! */
	while (true) {
		dpp::cluster bot(token, intents, dev ? 1 : 2, 0, 1, true);

		/* Set up spdlog logger */
		std::shared_ptr<spdlog::logger> log;
		spdlog::init_thread_pool(8192, 2);
		std::vector<spdlog::sink_ptr> sinks;
		auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
		auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("sporks.log", 1024 * 1024 * 5, 10);
		sinks.push_back(stdout_sink);
		sinks.push_back(rotating);
		log = std::make_shared<spdlog::async_logger>("test", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
		spdlog::register_logger(log);
		log->set_pattern("%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v");
		log->set_level(spdlog::level::level_enum::debug);	

		/* Integrate spdlog logger to D++ log events */
		bot.on_log([&bot, &log](const dpp::log_t & event) {
			switch (event.severity) {
				case dpp::ll_trace:
					log->trace("{}", event.message);
				break;
				case dpp::ll_debug:
					log->debug("{}", event.message);
				break;
				case dpp::ll_info:
					log->info("{}", event.message);
				break;
				case dpp::ll_warning:
					log->warn("{}", event.message);
				break;
				case dpp::ll_error:
					log->error("{}", event.message);
				break;
				case dpp::ll_critical:
				default:
					log->critical("{}", event.message);
				break;
			}
		});

		Bot client(dev, test, members, &bot);

		/* Attach events to the Bot class methods */
		bot.on_message_create(std::bind(&Bot::onMessage, &client, std::placeholders::_1));
		bot.on_ready(std::bind(&Bot::onReady, &client, std::placeholders::_1));
		bot.on_channel_create(std::bind(&Bot::onChannel, &client, std::placeholders::_1));
		bot.on_guild_member_add(std::bind(&Bot::onMember, &client, std::placeholders::_1));
		bot.on_guild_create(std::bind(&Bot::onServer, &client, std::placeholders::_1));
		bot.on_guild_delete(std::bind(&Bot::onServerDelete, &client, std::placeholders::_1));
		bot.on_channel_delete(std::bind(&Bot::onChannelDelete, &client, std::placeholders::_1));
		bot.on_typing_start(std::bind(&Bot::onTypingStart, &client, std::placeholders::_1));
		bot.on_message_update(std::bind(&Bot::onMessageUpdate, &client, std::placeholders::_1));
		bot.on_message_delete(std::bind(&Bot::onMessageDelete, &client, std::placeholders::_1));
		bot.on_message_delete_bulk(std::bind(&Bot::onMessageDeleteBulk, &client, std::placeholders::_1));
		bot.on_guild_update(std::bind(&Bot::onGuildUpdate, &client, std::placeholders::_1));
		bot.on_message_reaction_add(std::bind(&Bot::onMessageReactionAdd, &client, std::placeholders::_1));
		bot.on_message_reaction_remove(std::bind(&Bot::onMessageReactionRemove, &client, std::placeholders::_1));
		bot.on_message_reaction_remove_all(std::bind(&Bot::onMessageReactionRemoveAll, &client, std::placeholders::_1));
		bot.on_user_update(std::bind(&Bot::onUserUpdate, &client, std::placeholders::_1));
		bot.on_resumed(std::bind(&Bot::onResumed, &client, std::placeholders::_1));
		bot.on_channel_update(std::bind(&Bot::onChannelUpdate, &client, std::placeholders::_1));
		bot.on_channel_pins_update(std::bind(&Bot::onChannelPinsUpdate, &client, std::placeholders::_1));
		bot.on_guild_ban_add(std::bind(&Bot::onGuildBanAdd, &client, std::placeholders::_1));
		//bot.on_guild_ban_remove(std::bind(&Bot::onGuildBanRemove, &client, std::placeholders::_1));
		bot.on_guild_emojis_update(std::bind(&Bot::onGuildEmojisUpdate, &client, std::placeholders::_1));
		bot.on_guild_integrations_update(std::bind(&Bot::onGuildIntegrationsUpdate, &client, std::placeholders::_1));
		bot.on_guild_member_remove(std::bind(&Bot::onGuildMemberRemove, &client, std::placeholders::_1));
		bot.on_guild_member_update(std::bind(&Bot::onGuildMemberUpdate, &client, std::placeholders::_1));
		bot.on_guild_members_chunk(std::bind(&Bot::onGuildMembersChunk, &client, std::placeholders::_1));
		bot.on_guild_role_create(std::bind(&Bot::onGuildRoleCreate, &client, std::placeholders::_1));
		bot.on_guild_role_update(std::bind(&Bot::onGuildRoleUpdate, &client, std::placeholders::_1));
		bot.on_guild_role_delete(std::bind(&Bot::onGuildRoleDelete, &client, std::placeholders::_1));
		bot.on_presence_update(std::bind(&Bot::onPresenceUpdate, &client, std::placeholders::_1));
		bot.on_voice_state_update(std::bind(&Bot::onVoiceStateUpdate, &client, std::placeholders::_1));
		bot.on_voice_server_update(std::bind(&Bot::onVoiceServerUpdate, &client, std::placeholders::_1));
		bot.on_webhooks_update(std::bind(&Bot::onWebhooksUpdate, &client, std::placeholders::_1));
	
		try {
			/* Actually connect and start the event loop */
			bot.start(false);
		}
		catch (std::exception e) {
			bot.log(dpp::ll_error, fmt::format("Oof! {}", e.what()));
		}

		/* Reconnection delay to prevent hammering discord */
		::sleep(30);
	}
}

