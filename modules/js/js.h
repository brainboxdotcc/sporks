/************************************************************************************
 * 
 * Sporks, the learning, scriptable Discord bot!
 *
 * Copyright 2019 Craig Edwards <support@sporks.gg>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/

#pragma once
#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "duktape.h"
#include <sporks/modules.h>

using json = nlohmann::json; 

class JS {
	std::string lasterror;
	dpp::cluster* core;
	class Bot* bot;
	std::thread* web_request_watcher;
	std::mutex jsmutex;
	bool terminate;
public:
	JS(class dpp::cluster* _core, class Bot* bot);
	~JS();
	bool run(uint64_t channel_id, const std::unordered_map<std::string, json> &vars, const std::string &callback_fn = "", const std::string &callback_content = "");
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
        virtual bool OnMessage(const dpp::message_create_t &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions);
};

