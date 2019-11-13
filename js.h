#include <unordered_map>
#include <string>
#include <vector>
#include "duktape.h"
#include <spdlog/spdlog.h>
#include <aegis.hpp>

using json = nlohmann::json; 

class JS {
	std::string lasterror;
	std::shared_ptr<spdlog::logger>& log;
	class Bot* bot;
public:
	JS(std::shared_ptr<spdlog::logger>& logger, class Bot* bot);
	bool run(int64_t channel_id, const std::unordered_map<std::string, json> &vars);
	bool hasReplied();
};
