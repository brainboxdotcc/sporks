#include <string>
#include <mutex>
#include <string>
#include "bot.h"
#include "queue.h"

void send_responses(Bot* client, std::mutex* output_mutex, Queue* outputs) {
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
			client->sendMessage(done.front().channelID, done.front().message);
			done.pop();
		}
	}
}

