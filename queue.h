#pragma once

#include <string>
#include <queue>

class QueueItem
{
  public:
	  std::string channelID;
	  std::string username;
	  std::string message;
};

typedef std::queue<QueueItem> Queue;
