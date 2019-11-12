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
public:
	JS(std::shared_ptr<spdlog::logger>& logger);
	bool run(int64_t channel_id, const std::unordered_map<std::string, json> &vars);
};
