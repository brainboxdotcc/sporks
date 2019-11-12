#pragma once

#include <string>
#include <queue>
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
	std::unordered_map<std::string, json> jsonstore;
};

typedef std::queue<QueueItem> Queue;
