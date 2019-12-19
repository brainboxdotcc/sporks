#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "duktape.h"
#include <spdlog/spdlog.h>
#include <aegis.hpp>
#include <sporks/modules.h>

using json = nlohmann::json; 

class JS {
	std::string lasterror;
	std::shared_ptr<spdlog::logger>& log;
	class Bot* bot;
	std::thread* web_request_watcher;
	std::mutex jsmutex;
	bool terminate;
public:
	JS(std::shared_ptr<spdlog::logger>& logger, class Bot* bot);
	~JS();
	bool run(int64_t channel_id, const std::unordered_map<std::string, json> &vars, const std::string &callback_fn = "", const std::string &callback_content = "");
	void WebRequestWatch();
	bool hasReplied();
	bool channelHasJS(int64_t channel_id);
};

class JSModule : public Module
{
	JS* js;
public:
        JSModule(Bot* instigator, ModuleLoader* ml);
        virtual ~JSModule();
        virtual std::string GetVersion();
        virtual std::string GetDescription();
        virtual bool OnMessage(const aegis::gateway::events::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions);
};

