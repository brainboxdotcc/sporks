#pragma once

#include <string>
#include <deque>
#include <aegis.hpp>
#include <unordered_map>

using json = nlohmann::json;

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
	bool tombstone;
};

typedef std::deque<QueueItem> Queue;
