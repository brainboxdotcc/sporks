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

#include "infobot.h"
#include <sporks/bot.h>
#include <sporks/includes.h>
#include "queue.h"
#include <sporks/config.h>
#include <sporks/stringops.h>
#include <sporks/modules.h>
#include <iostream>
#include <sstream>
#include <fmt/format.h>
#include <dpp/nlohmann/json.hpp>
#include "backend.h"

using json = nlohmann::json;

int InfobotModule::random(int min, int max)
{
	static bool first = true;
	if (first) {
		srand(time(NULL));
		first = false;
	}
	return min + rand() % (( max + 1 ) - min);
}

QueueStats InfobotModule::GetQueueStats() {
	QueueStats q;
	q.users = 0;
	q.guilds = 0;
	if (bot->counters.find("userqueue") != bot->counters.end()) {
		q.users = bot->counters["userqueue"];
	}
	if (bot->counters.find("guildqueue") != bot->counters.end()) {
		q.guilds = bot->counters["guildqueue"];
	}

	return q;
}

void InfobotModule::Input(QueueItem &query)
{
	/* Process anything in the inputs queue */
	bool has_item = false;
	/* Block to encapsulate lock_guard for input queue */
	json channel_settings = getSettings(bot, query.channelID, query.serverID);

	/* Process the input through to infobot backend if:
	 * A) the bot is directly mentioned, or,
	 * B) Learning is enabled for the channel (default for all channels)
	 */
	has_item = query.mentioned || settings::IsLearningEnabled(channel_settings);

	if (has_item) {
		/* Fix: If there isnt a list yet, don't try and do this otherwise it will result in a call of random(0, -1) and a SIGFPE */
		std::string randnick = "";
		if (nickList.find(query.serverID) != nickList.end() && this->nickList[query.serverID].size() > 0) {
			randnick =  this->nickList[query.serverID][random(0, this->nickList[query.serverID].size() - 1)];
		}

		/* Mangle common prefixes, so if someone asks "What is x" it is treated same as "x?" */
		std::string cleaned_message = query.message;
		std::vector<std::string> prefixes = {
			"what is a",
			"whats",
			"whos",
			"where's",
			"whats up with",
			"whats going off with",
			"what is",
			"tell me about",
			"who is",
			"what are",
			"who are",
			"wtf is",
			"tell me about",
			"tell me",
			"can someone help me with",
			"can you help me with",
			"can you help me",
			"can someone help me",
			"can i ask about",
			"can i ask",
			"do you",
			"can you",
			"will you",
			"wont you",
			"won't you",
			"how do i",
		};
		for (auto p = prefixes.begin(); p != prefixes.end(); ++p) {
			if (lowercase(trim(cleaned_message.substr(0, p->length()))) == *p) {
				cleaned_message = trim(cleaned_message.substr(p->length(), cleaned_message.length() - p->length()));
			}
		}
		infodef def;
		std::string text = infobot_response(bot->user.username, cleaned_message, query.username, randnick, query.channelID, def, query.mentioned, settings::IsTalkative(channel_settings));
		bool found = def.found;
		
		if (found || query.mentioned) {
			query.message = text;
			Output(query);
		}
	}
}

InfobotModule::InfobotModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml)
{
	ml->Attach({ I_OnMessage, I_OnGuildCreate }, this);
	infobot_init();
}

InfobotModule::~InfobotModule()
{
}

std::string InfobotModule::GetVersion()
{
	/* NOTE: This version string below is modified by a pre-commit hook on the git repository */
	std::string version = "$ModVer 20$";
	return "1.0." + version.substr(8,version.length() - 9);
}

std::string InfobotModule::GetDescription()
{
	return "Infobot learning and responses";
}

bool InfobotModule::OnGuildCreate(const dpp::guild_create_t &gc)
{
	this->nickList[gc.created->id] = std::vector<std::string>();
	for (auto i = gc.created->members.begin(); i != gc.created->members.end(); ++i) {
		dpp::user* u = dpp::find_user(i->second.user_id);
		if (u) {
			this->nickList[gc.created->id].push_back(u->username);
		}
	}
	return true;
}

bool InfobotModule::OnMessage(const dpp::message_create_t &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions)
{
	dpp::message_create_t msg = message;

	QueueItem query;
	query.message = clean_message;
	query.original_message = clean_message;
	query.channelID = msg.msg->channel_id;
	query.serverID = msg.msg->guild_id;
	query.username = msg.msg->author ? msg.msg->author->username : "";
	query.mentioned = mentioned;
	query.original_username = query.username;
	Input(query);

	return true;
}

ENTRYPOINT(InfobotModule);

