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

#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <aegis.hpp>
#include <sporks/modules.h>
#include "queue.h"

using json = nlohmann::json; 

typedef std::unordered_map<int64_t, std::vector<std::string>> RandomNickCache;

struct QueueStats {
	size_t inputs;
	size_t outputs;
	size_t users;
};

/**
 * Infobot module: Allows smart responses from the botnix/infobot.pm system.
 */
class InfobotModule : public Module
{
	/**
	 * The nickname of the bot on IRC as reported by botnix
	 */
	std::string core_nickname;

	/**
	 * Threads
	 */
	std::thread* thr_input;
	std::thread* thr_output;

	/**
	 *  Thread safety for queues
	 */
	std::mutex input_mutex;
	std::mutex output_mutex;

	/** Set to true in destructor when threads are to terminte
	 */
	bool terminate;	

	/**
	 *  Input and output queue, lists of messages awaiting processing, or to be sent to channels
	 */
	Queue inputs;
	Queue outputs;

	/**
	 *  Contains a vector of nicknames per-server for selecting a random nickname only
	 */
	RandomNickCache nickList;

	/**
	 * Read line from a socket
	 */
	size_t readLine(int fd, char *buffer, size_t n);
	/**
	 * Write line to a socket 
	 */
	bool writeLine(int fd, const std::string &str);

	/**
	 * Report bot status as an embed
	 */
	void ShowStatus(const std::vector<std::string> &matches, int64_t channelID);

	/**
	 * Get queue sizes for use in status report
	 */
	QueueStats GetQueueStats();

public:
	InfobotModule(Bot* instigator, ModuleLoader* ml);
	virtual ~InfobotModule();
	virtual std::string GetVersion();
	virtual std::string GetDescription();

	virtual bool OnMessage(const aegis::gateway::events::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions);
	virtual bool OnGuildCreate(const aegis::gateway::events::guild_create &gc);
	virtual bool OnGuildDelete(const aegis::gateway::events::guild_delete &guild);

	/**
	 * Set the core IRC nickname used for queries to botnix
	 */
	void set_core_nickname(const std::string &coredata);

	/**
	 * Random integer in range
	 */
	int random(int min, int max);

	/* Thread handlers */
	void InputThread();	     /* Processes input lines from channel messages, complex responses can take upwards of 250ms */
	void OutputThread();	    /* Outputs lines due to be sent to channel messages, after being processed by the input thread */
};

