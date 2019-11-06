#include "rss.h"

int64_t GetRSS() {
	int64_t ram = 0;
	std::ifstream self_status("/proc/self/status");
	while (self_status) {
		std::string token;
		self_status >> token;
		if (token == "VmRSS:") {
			self_status >> ram;
			break;
		}
	}
	self_status.close();
	return ram;
}
