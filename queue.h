#pragma once

#include <string>
#include <deque>
#include <aegis.hpp>
#include <unordered_map>

using json = nlohmann::json;

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
