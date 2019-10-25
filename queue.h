#pragma once

#include <string>
#include <queue>

class QueueItem
{
  public:
	  std::string channelID;
	  std::string serverID;
	  std::string username;
	  std::string message;
	  bool mentioned;
};

typedef std::queue<QueueItem> Queue;
