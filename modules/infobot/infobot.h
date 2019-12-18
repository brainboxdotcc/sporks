#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <aegis.hpp>
#include "../../modules.h"
#include "../../queue.h"

using json = nlohmann::json; 

typedef std::unordered_map<int64_t, std::vector<std::string>> RandomNickCache;

struct QueueStats {
	size_t inputs;
	size_t outputs;
	size_t users;
};

class InfobotModule : public Module
{
	std::string core_nickname;

	/* Threads */
	std::thread* thr_input;
	std::thread* thr_output;

	/* Thread safety for caches and queues */
	std::mutex input_mutex;
	std::mutex output_mutex;

	bool terminate;	

	/* Input and output queue, lists of messages awaiting processing, or to be sent to channels */
	Queue inputs;
	Queue outputs;

	RandomNickCache nickList;       /* Special case, contains a vector of nicknames per-server for selecting a random nickname only */

	size_t readLine(int fd, char *buffer, size_t n);
	bool writeLine(int fd, const std::string &str);

	void ShowStatus(const std::vector<std::string> &matches, int64_t channelID);

	QueueStats GetQueueStats();

public:
	InfobotModule(Bot* instigator, ModuleLoader* ml);
	virtual ~InfobotModule();
	virtual std::string GetVersion();
	virtual std::string GetDescription();

	virtual bool OnMessage(const aegis::gateway::events::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions);
	virtual bool OnGuildCreate(const aegis::gateway::events::guild_create &gc);
	virtual bool OnGuildDelete(const aegis::gateway::events::guild_delete &guild);

	void set_core_nickname(const std::string &coredata);
	int random(int min, int max);

	/* Thread handlers */
	void InputThread();	     /* Processes input lines from channel messages, complex responses can take upwards of 250ms */
	void OutputThread();	    /* Outputs lines due to be sent to channel messages, after being processed by the input thread */
};

