#pragma once

#include <string>
#include <queue>
#include <mutex>
#include "bot.h"
#include "queue.h"

void send_responses(Bot* client, std::mutex* output_mutex, std::mutex* channel_hash_mutex, Queue* outputs);