#pragma once

#include <string>
#include <queue>

class QueueItem
{
  public:
	  int64_t channelID;
	  int64_t serverID;
	  std::string username;
	  std::string message;
	  bool mentioned;
};

typedef std::queue<QueueItem> Queue;
