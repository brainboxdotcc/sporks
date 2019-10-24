#include <string>
#include <mutex>
#include <string>
#include "bot.h"
#include "queue.h"
#include "config.h"

void send_responses(Bot* client, std::mutex* output_mutex, std::mutex* channel_hash_mutex, Queue* outputs) {
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		std::queue<QueueItem> done;
		do {
			std::lock_guard<std::mutex> output_lock(*output_mutex);
			while (!outputs->empty()) {
				done.push(outputs->front());
				outputs->pop();
			}
		} while (false);
		while (!done.empty()) {
			SleepyDiscord::Channel channel;
			rapidjson::Document channel_settings;
			do {
				std::lock_guard<std::mutex> hash_lock(*channel_hash_mutex);
				channel = client->channelList.find(done.front().channelID)->second;
				channel_settings = getSettings(client, channel);
			} while (false);
			if (settings::IsTalkative(channel_settings)) {
				client->sendMessage(channel.ID, done.front().message);
			}
			done.pop();
		}
	}
}

