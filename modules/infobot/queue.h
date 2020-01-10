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

#pragma once
#include <string>
#include <deque>
#include <aegis.hpp>
#include <unordered_map>

using json = nlohmann::json;

struct QueueStats {
	uint64_t users;
};

/**
 * Contains an queued input or output item.
 * These are messages in and out of the bot, they are queued
 * because the request to process them can take some time and
 * has to pass to an external program.
 */
class QueueItem
{
  public:
	int64_t channelID;
	int64_t serverID;
	std::string username;
	std::string message;
	bool mentioned;
};

