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
#include <dpp/dpp.h>
#include <dpp/json_fwd.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sporks/modules.h>
#include "queue.h"
#include "backend.h"

using json = nlohmann::json; 

typedef std::unordered_map<int64_t, std::vector<std::string>> RandomNickCache;

/**
 * Infobot module: Allows smart responses from the botnix/infobot.pm system.
 */
class InfobotModule : public Module
{
	/**
	 *  Contains a vector of nicknames per-server for selecting a random nickname only
	 */
	RandomNickCache nickList;

	/**
	 * Report bot status as an embed
	 */
	void ShowStatus(int days, int hours, int minutes, int seconds, uint64_t db_changes, uint64_t questions, uint64_t facts, time_t startup, int64_t channelID);

	/**
	 * Get queue sizes for use in status report
	 */
	QueueStats GetQueueStats();

	void infobot_init();
	std::string infobot_response(std::string mynick, std::string otext, std::string usernick, std::string randuser, int64_t channelID, infodef &def, bool mentioned, bool talkative);

	void ProcessEmbed(const std::string &embed_json, int64_t channelID);
	void EmbedWithFields(const std::string &title, std::map<std::string, std::string> fields, int64_t channelID);

public:
	InfobotModule(Bot* instigator, ModuleLoader* ml);
	virtual ~InfobotModule();
	virtual std::string GetVersion();
	virtual std::string GetDescription();

	virtual bool OnMessage(const dpp::message_create_t &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions);
	virtual bool OnGuildCreate(const dpp::guild_create_t &gc);

	/**
	 * Random integer in range
	 */
	int random(int min, int max);

	/* Thread handlers */
	void Input(QueueItem &queueitem);		/* Processes input lines from channel messages */
	void Output(QueueItem &queueitem);	/* Outputs lines due to be sent to channel messages */
};

