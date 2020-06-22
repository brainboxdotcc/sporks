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

#include "js.h"
#include <sporks/bot.h>
#include <sporks/config.h>
#include <sporks/stringops.h>
#include <sporks/database.h>
#include <sporks/modules.h>
#include <thread>
#include <streambuf>
#include <fstream>
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

extern int interrupt;
int64_t current_context;
aegis::guild* current_guild;
static std::shared_ptr<spdlog::logger> c_apis_suck;
std::unordered_map<int64_t, duk_context*> emptyref;
std::unordered_map<int64_t, duk_context*> &contexts = emptyref;
std::unordered_map<int64_t, size_t> total_allocated;
static Bot* botref;



const size_t max_allocated_unvoted = 256 * 1024;
const size_t max_allocated_voted = 512 * 1024;
const uint64_t timeout_unvoted = 10;
const uint64_t timeout_voted = 20;



uint64_t timeout = timeout_unvoted;
const uint32_t message_limit = 5;

uint32_t message_total = 0;
extern timeval t_script_start;
extern timeval t_script_now;

void sandbox_fatal(void *udata, const char *msg);
void sandbox_free(void *udata, void *ptr);
void *sandbox_alloc(void *udata, duk_size_t size);
static void *sandbox_realloc(void *udata, void *ptr, duk_size_t size);
std::string Sanitise(const std::string &s);

struct program
{
	std::string name;
	std::string source;
};

std::unordered_map<int64_t, program> code;

struct alloc_hdr {
	/* The double value in the union is there to ensure alignment is
	 * good for IEEE doubles too.  In many 32-bit environments 4 bytes
	 * would be sufficiently aligned and the double value is unnecessary.
	 */
	union {
		size_t sz;
		double d;
	} u;
};

static size_t max_allocated = max_allocated_unvoted;  /* 256kB sandbox */


class ExitException : public std::exception {
};

static void define_func(duk_context* ctx, const std::string &name, duk_c_function func, int nargs)
{
	duk_push_string(ctx, name.c_str());
	duk_push_c_function(ctx, func, nargs);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
}

static void define_number(duk_context* ctx, const std::string &name, duk_double_t num)
{
	duk_push_string(ctx, name.c_str());
	duk_push_number(ctx, num);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
}

static void define_string(duk_context* ctx, const std::string &name, const std::string &value)
{
	duk_push_string(ctx, name.c_str());
	duk_push_string(ctx, value.c_str());
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
}

static duk_ret_t js_print(duk_context *cx)
{
	int argc = duk_get_top(cx);
	std::string output;
	if (argc < 1)
		return 0;
	for (int i = 0; i < argc; i++)
		output.append(duk_to_string(cx, i - argc)).append(" ");
	c_apis_suck->debug("JS debuglog(): {}", trim(output));
	return 0;
}

static void duk_build_object(duk_context* cx, const std::map<std::string, std::string> &strings, const std::map<std::string, bool> &bools)
{
	duk_idx_t obj_idx = duk_push_bare_object(cx);
	for (auto i = strings.begin(); i != strings.end(); ++i) {
		duk_push_string(cx, i->first.c_str());
		duk_push_string(cx, i->second.c_str());
		duk_put_prop(cx, obj_idx);
	}
	for (auto i = bools.begin(); i != bools.end(); ++i) {
		duk_push_string(cx, i->first.c_str());
		duk_push_boolean(cx, i->second);
		duk_put_prop(cx, obj_idx);
	}
}

static duk_ret_t js_create_message(duk_context *cx)
{
	int argc = duk_get_top(cx);
	std::string output;
	if (argc < 2)
		return 0;
	if (!duk_is_string(cx, 0)) {
		c_apis_suck->warn("JS create_message(): parameter 1 is not a string");
		return 0;
	}
	std::string id = duk_get_string(cx, 0);
	aegis::channel* c = current_guild->find_channel(from_string<int64_t>(id, std::dec));
	if (c) {
		for (int i = 1; i < argc; i++) {
			output.append(duk_to_string(cx, i - argc)).append(" ");
		}
		std::string message = trim(output);
		if (message_total >= message_limit) {
			duk_push_error_object(cx, DUK_ERR_RANGE_ERROR, "Message limit reached");
			return duk_throw(cx);
		}
		if (!botref->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == c->get_guild().get_id()) {
			c->create_message(Sanitise(message));
			botref->sent_messages++;
		}
		message_total++;
		c_apis_suck->debug("JS create_message() on guild={}/channel={}: {}", current_guild->get_id(), id, message);
	} else {
		c_apis_suck->warn("JS create_message(): invalid channel id: {}", id);
	}
	return 0;
}

static duk_ret_t js_add_reaction(duk_context *cx)
{
	int argc = duk_get_top(cx);
	std::string output;
	if (argc < 3)
		return 0;
	if (!duk_is_string(cx, 0)) {
		c_apis_suck->warn("JS add_reaction(): parameter 1 is not a string");
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS add_reaction(): parameter 2 is not a string");
		return 0;
	}
	if (!duk_is_string(cx, -2)) {
		c_apis_suck->warn("JS add_reaction(): parameter 3 is not a string");
		return 0;
	}	
	std::string id = duk_get_string(cx, 0);
	std::string message_id = duk_get_string(cx, -2);
	std::string emoji = duk_get_string(cx, -1);
	aegis::channel* c = current_guild->find_channel(from_string<int64_t>(id, std::dec));
	if (c) {
		c->create_reaction(from_string<int64_t>(message_id, std::dec), trim(emoji));
		c_apis_suck->debug("JS add_reaction() on guild={}/channel={}: msg id={} emoji={}", current_guild->get_id(), id, message_id, emoji);
	} else {
		c_apis_suck->warn("JS add_reaction(): invalid channel id: {}", id);
	}
	return 0;
}

static duk_ret_t js_delete_reaction(duk_context *cx)
{
	int argc = duk_get_top(cx);
	std::string output;
	if (argc < 3)
		return 0;
	if (!duk_is_string(cx, 0)) {
		c_apis_suck->warn("JS delete_reaction(): parameter 1 is not a string");
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS delete_reaction(): parameter 2 is not a string");
		return 0;
	}
	if (!duk_is_string(cx, -2)) {
		c_apis_suck->warn("JS delete_reaction(): parameter 3 is not a string");
		return 0;
	}
	std::string id = duk_get_string(cx, 0);
	std::string message_id = duk_get_string(cx, -2);
	std::string emoji = duk_get_string(cx, -1);
	aegis::channel* c = current_guild->find_channel(from_string<int64_t>(id, std::dec));
	if (c) {
		c->delete_own_reaction(from_string<int64_t>(message_id, std::dec), trim(emoji));
		c_apis_suck->debug("JS delete_reaction() on guild={}/channel={}: msg_id={} emoji={}", current_guild->get_id(), id, message_id, emoji);
	} else {
		c_apis_suck->warn("JS delete_reaction(): invalid channel id: {}", id);
	}
	return 0;
}

std::string Sanitise(const std::string &s) {
	return ReplaceString(ReplaceString(s, "@here", "@‎here"), "@everyone", "@‎everyone");
}

static duk_ret_t js_create_embed(duk_context *cx)
{
	int argc = duk_get_top(cx);
	std::string output;
	if (argc != 2)
		return 0;
	if (!duk_is_string(cx, 0)) {
		c_apis_suck->warn("JS create_embed(): parameter 1 is not a string");
		return 0;
	}
	if (!duk_is_object(cx, -1)) {
		c_apis_suck->warn("JS create_embed(): parameter 2 is not an object");
		return 0;
	}
	std::string id = duk_get_string(cx, 0);
	std::string j = duk_json_encode(cx, -1);
	aegis::channel* c = current_guild->find_channel(from_string<int64_t>(id, std::dec));
	if (c) {
		json embed;
		try {
			embed = json::parse(Sanitise(j));
			if (message_total >= message_limit) {
				duk_push_error_object(cx, DUK_ERR_RANGE_ERROR, "Message limit reached");
				return duk_throw(cx);
			}
			if (!botref->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == c->get_guild().get_id()) {
				c->create_message_embed("", embed);
				botref->sent_messages++;
			}
			message_total++;
			c_apis_suck->debug("JS create_embed() on guild={}/channel={}: {}", current_guild->get_id(), id, j);
		} catch (const std::exception &e) {
			c_apis_suck->error("JS create_embed() JSON parse exception {}", e.what());
		}
	} else {
		c_apis_suck->warn("JS create_message(): invalid channel id: {}", id);
	}
	return 0;
}

void do_web_request(const std::string &reqtype, const std::string &url, const std::string &callback, const std::string &postdata = "")
{
	
	db::resultset rs = db::query("SELECT count(guild_id) AS count1 FROM infobot_web_requests WHERE guild_id = ?", {std::to_string(current_guild->get_id())});
	if (rs[0]["count1"] == "0") {
		db::resultset rs = db::query("SELECT count(channel_id) AS count2 FROM infobot_web_requests WHERE guild_id = ?", {std::to_string(current_context)});
		if (rs[0]["count2"] == "0") {
			c_apis_suck->debug("JS web request created on guild={}/channel={}: {}", current_guild->get_id(), current_context, url);
			db::query("INSERT INTO infobot_web_requests (channel_id, guild_id, url, type, postdata, callback) VALUES('?','?','?','?','?','?')",
				{std::to_string(current_context), std::to_string(current_guild->get_id()), url, reqtype, postdata, callback});
		}
	}
}

static duk_ret_t js_get(duk_context *cx)
{
	/* url, callback */
	int argc = duk_get_top(cx);
	if (argc != 2) {
		c_apis_suck->warn("JS get(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, 0) || !duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS post(): parameters are not strings!");
		return 0;
	}
	std::string url = duk_get_string(cx, 0);
	std::string callback = duk_get_string(cx, -1);
	do_web_request("GET", url, callback);
	return 0;
}

static duk_ret_t js_post(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 3) {
		c_apis_suck->warn("JS post(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, 0) || !duk_is_string(cx, -1) || !duk_is_string(cx, -2)) {
		c_apis_suck->warn("JS post(): parameters are not strings!");
		return 0;
	}
	std::string url = duk_get_string(cx, 0);
	std::string postdata = duk_get_string(cx, -1);
	std::string callback = duk_get_string(cx, -2);
	do_web_request("POST", url, callback, postdata);
	return 0;
}

static duk_ret_t js_find_user(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 1) {
		c_apis_suck->warn("JS find_user(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS find_user(): parameter is not a string");
		return 0;
	}
	std::string id = duk_get_string(cx, -1);
	aegis::user* u = current_guild->find_member(from_string<int64_t>(id, std::dec));
	if (u) {
		std::string nickname = u->get_name(current_guild->get_id());
		duk_build_object(cx, {
			{ "id", std::to_string(u->get_id()) },
			{ "username", u->get_username() },
			{ "discriminator", std::to_string(u->get_discriminator()) },
			{ "avatar", u->get_avatar() },
			{ "mention", u->get_mention() },
			{ "full_name", u->get_full_name() },
			{ "nickname", nickname }
		}, {
			{ "bot", u->is_bot() },
			{ "mfa_enabled", u->is_mfa_enabled() }
		});
		return 1;
	}
	return 0;
}

static duk_ret_t js_find_username(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 1) {
		c_apis_suck->warn("JS find_username(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS find_username(): parameter is not a string");
		return 0;
	}
	std::string username = lowercase(std::string(duk_get_string(cx, -1)));
	/* Yeah this sucks and is O(n). Until aegis provides a better way of looking up a guild member by
	 * anything other than snowflake id, this will have to do
	 */
	for (auto u = current_guild->get_members().begin(); u != current_guild->get_members().end(); ++u) {
		if (lowercase(u->second->get_full_name()) == username) {
			std::string nickname = u->second->get_name(current_guild->get_id());
			duk_build_object(cx, {
				{ "id", std::to_string(u->second->get_id()) },
				{ "username", u->second->get_username() },
				{ "discriminator", std::to_string(u->second->get_discriminator()) },
				{ "avatar", u->second->get_avatar() },
				{ "mention", u->second->get_mention() },
				{ "full_name", u->second->get_full_name() },
				{ "nickname", nickname }
			}, {
				{ "bot", u->second->is_bot() },
				{ "mfa_enabled", u->second->is_mfa_enabled() }
			});
			return 1;
		}
	}
	return 0;
}

static duk_ret_t js_find_channel(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 1) {
		c_apis_suck->warn("JS find_channel(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS find_channel(): parameter is not a string");
		return 0;
	}
	std::string id = duk_get_string(cx, -1);
	aegis::channel* c = current_guild->find_channel(from_string<int64_t>(id, std::dec));
	if (c) {
		duk_build_object(cx, {
			{ "id", std::to_string(c->get_id()) },
			{ "name", c->get_name() },
			{ "type", std::to_string(c->get_type()) },
			{ "guild_id", std::to_string(c->get_guild_id()) },
			{ "parent_id", std::to_string(c->get_parent_id()) }
		}, {
			{ "dm", c->is_dm() },
			{ "nsfw", c->nsfw() }
		});
		return 1;
	}
	return 0;
}

static duk_ret_t js_load(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 1) {
		c_apis_suck->warn("JS load(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS load(): parameter is not a string");
		return 0;
	}
	std::string keyname = duk_get_string(cx, -1);
	std::string guild_id = std::to_string(current_guild->get_id());
	db::resultset rs = db::query("SELECT value FROM infobot_javascript_kv WHERE guild_id = ? AND keyname = '?'", {guild_id, keyname});
	if (rs.size() == 1 && rs[0].find("value") != rs[0].end()) {
		duk_push_string(cx, rs[0].find("value")->second.c_str());
		return 1;
	} else {
		return 0;
	}
}

static duk_ret_t js_delete(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 1) {
		c_apis_suck->warn("JS delete(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS delete(): parameter is not a string");
		return 0;
	}
	std::string keyname = duk_get_string(cx, -1);
	std::string guild_id = std::to_string(current_guild->get_id());
	db::query("DELETE FROM infobot_javascript_kv WHERE guild_id = ? AND keyname = '?'", {guild_id, keyname});
	return 0;
}

static duk_ret_t js_save(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 2) {
		c_apis_suck->warn("JS save(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, 0)) {
		c_apis_suck->warn("JS save(): first parameter is not a string");
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS save(): second parameter is not a string");
		return 0;
	}
	std::string keyname = duk_get_string(cx, 0);
	std::string value = duk_get_string(cx, -1);
	std::string guild_id = std::to_string(current_guild->get_id());
	db::query("INSERT INTO infobot_javascript_kv (guild_id, keyname, value) VALUES(?,'?','?') ON DUPLICATE KEY UPDATE value ='?'", {guild_id, keyname, value, value});
	return 0;
}

static duk_ret_t js_exit(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 1) {
		c_apis_suck->warn("JS exit(): incorrect number of parameters: {}", argc);
	}
	throw ExitException();
}

static duk_ret_t js_find_channelname(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc != 1) {
		c_apis_suck->warn("JS find_channelname(): incorrect number of parameters: {}", argc);
		return 0;
	}
	if (!duk_is_string(cx, -1)) {
		c_apis_suck->warn("JS find_channelname(): parameter is not a string");
		return 0;
	}
	std::string channelname = lowercase(std::string(duk_get_string(cx, -1)));
	for (auto c = current_guild->get_channels().begin(); c != current_guild->get_channels().end(); ++c) {
		if (lowercase(c->second->get_name()) == channelname) {
			duk_build_object(cx, {
				{ "id", std::to_string(c->second->get_id()) },
				{ "name", c->second->get_name() },
				{ "type", std::to_string(c->second->get_type()) },
				{ "guild_id", std::to_string(c->second->get_guild_id()) },
				{ "parent_id", std::to_string(c->second->get_parent_id()) }
			}, {
				{ "dm", c->second->is_dm() },
				{ "nsfw", c->second->nsfw() }
			});
			return 1;
		}
	}
	return 0;
}

JS::JS(std::shared_ptr<spdlog::logger>& logger, Bot* thisbot) : log(logger), bot(thisbot)
{
	terminate = false;
	web_request_watcher = new std::thread(&JS::WebRequestWatch, this);
}

void JS::WebRequestWatch()
{
	while (!this->terminate)
	{
		db::resultset rs = db::query("SELECT * FROM infobot_web_requests WHERE statuscode != '000'", {});
		for (auto i = rs.begin(); i != rs.end(); ++i) {
			c_apis_suck->debug("JS web request response received for url {}", (*i)["url"]);
			run(from_string<int64_t>((*i)["channel_id"], std::dec), {}, (*i)["callback"], (*i)["returndata"]);
			db::query("DELETE FROM infobot_web_requests WHERE channel_id = ?", {(*i)["channel_id"]});
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

JS::~JS()
{
	terminate = true;
	if (web_request_watcher->joinable()) {
		web_request_watcher->join();
	}
	delete web_request_watcher;
}

bool JS::channelHasJS(int64_t channel_id)
{
	db::resultset r = db::query("SELECT id FROM infobot_discord_javascript WHERE id = ?", {std::to_string(channel_id)});
	/* No javascript configuration for this channel */
	if (r.size() == 0) {
		return false;
	} else {
		return true;
	}
}

bool JS::hasReplied()
{
	return message_total > 0;
}

std::string CleanErrorMessage(const std::string &error) {
	return ReplaceString(error, "    at [anon] (duk_js_var.c:1234) internal\n", "");
}

bool JS::run(int64_t channel_id, const std::unordered_map<std::string, json> &vars, const std::string &callback_fn, const std::string &callback_content)
{
	std::lock_guard<std::mutex> input_lock(this->jsmutex);
	duk_int_t ret;
	int i;

	aegis::channel* c = bot->core.find_channel(channel_id);
	if (!c) {
		log->error("JS::run() Can't find channel {}", channel_id);
		return false;
	}
	current_guild = &c->get_guild();

	/* Check if a user has a current vote in the system that is valid for the past day. If they do, boost their quotas for cpu time and ram usage. */
	db::resultset vrs = db::query("SELECT * FROM `infobot_votes` WHERE vote_time > now() - INTERVAL 1 DAY AND snowflake_id = '?'", {std::to_string(current_guild->get_owner())});
	if (vrs.size() > 0) {
		/* User has voted, increase their allowances */
		timeout = timeout_voted;
		max_allocated = max_allocated_voted;
	} else {
		/* Standard allowances */
		timeout = timeout_unvoted;
		max_allocated = max_allocated_unvoted;
	}

	auto iter = code.find(channel_id);
	duk_context* ctx;
	program v;

	c_apis_suck = log;
	botref = bot;

	if (iter == code.end() || settings::getJSConfig(channel_id, "dirty") == "1") {

		log->info("create new context for channel {} due to reload request", channel_id);
		std::string source = settings::getJSConfig(channel_id, "script");
		std::string name = std::to_string(channel_id) + ".js";
		v.name = name;
		v.source = source;

		code[channel_id] = v;

		settings::setJSConfig(channel_id, "dirty", "0");

	} else {
		v = code[channel_id];
	}

	current_context = channel_id;
	total_allocated[channel_id] = 0;
	message_total = 0;

	auto t_start = std::chrono::high_resolution_clock::now();	
	ctx = duk_create_heap(sandbox_alloc, sandbox_realloc, sandbox_free, NULL, sandbox_fatal);

	duk_push_global_object(ctx);
	define_string(ctx, "CHANNEL_ID", std::to_string(channel_id));
	define_string(ctx, "GUILD_ID", std::to_string(current_guild->get_id()));
	define_string(ctx, "BOT_ID", std::to_string(bot->getID()));
	define_func(ctx, "debuglog", js_print, DUK_VARARGS);
	define_func(ctx, "find_user", js_find_user, 1);
	define_func(ctx, "find_channel", js_find_channel, 1);
	define_func(ctx, "create_message", js_create_message, DUK_VARARGS);
	define_func(ctx, "create_embed", js_create_embed, 2);
	define_func(ctx, "find_username", js_find_username, 1);
	define_func(ctx, "find_channelname", js_find_channelname, 1);
	define_func(ctx, "save", js_save, 2);
	define_func(ctx, "load", js_load, 1);
	define_func(ctx, "delete", js_delete, 1);
	define_func(ctx, "get", js_get, 2);
	define_func(ctx, "post", js_post, 3);
	define_func(ctx, "exit", js_exit, 1);
	define_func(ctx, "add_reaction", js_add_reaction, 3);
	define_func(ctx, "delete_reaction", js_delete_reaction, 3);
	if (!callback_content.empty()) {
		define_string(ctx, "WCB_CONTENT", callback_content);
	}
	duk_pop(ctx);

	duk_push_string(ctx, v.name.c_str());
	std::string source;
	for (auto i = vars.begin(); i != vars.end(); ++i) {
		source += i->first + "=" + i->second.dump() + ";";
	}

	if (!callback_fn.empty()) {
		source += ";" + callback_fn + "(WCB_CONTENT);exit(0);" + v.source;
	} else {
		source += ";" + v.source;
	}

	if (duk_pcompile_string_filename(ctx, 0, source.c_str()) != 0) {
		lasterror = duk_safe_to_string(ctx, -1);
		log->error("couldnt compile: {}", lasterror);
		settings::setJSConfig(channel_id, "last_error", CleanErrorMessage(lasterror));
		auto t_end = std::chrono::high_resolution_clock::now();
		double compile_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
		settings::setJSConfig(channel_id, "last_compile_ms", std::to_string(compile_time_ms));
		duk_destroy_heap(ctx);
		return false;
	}

	auto t_end = std::chrono::high_resolution_clock::now();
	double compile_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
	settings::setJSConfig(channel_id, "last_compile_ms", std::to_string(compile_time_ms));

	if (!duk_is_function(ctx, -1)) {
		lasterror = "Top of stack is not a function";
		log->error("JS error: {}", lasterror);
		settings::setJSConfig(channel_id, "last_error", CleanErrorMessage(lasterror));
		duk_destroy_heap(ctx);
		return false;
	}

	interrupt = 0;
	ret = DUK_EXEC_SUCCESS;
	bool exited = false;
	try {
		gettimeofday(&t_script_start, nullptr);
		ret = duk_pcall(ctx, 0);
	}
	catch (const ExitException &e) {
		/* Graceful exit from javascript via an exception for the exit() function */
		ret = DUK_EXEC_SUCCESS;
		exited = true;
	}
	double exec_time_ms = (double)((t_script_now.tv_sec - t_script_start.tv_sec) * 1000000 + t_script_now.tv_usec - t_script_start.tv_usec) / 1000;
	settings::setJSConfig(channel_id, "last_exec_ms", std::to_string(exec_time_ms));
	settings::setJSConfig(channel_id, "last_memory_max", std::to_string(total_allocated[channel_id]));

	if (ret != DUK_EXEC_SUCCESS) {
		if (duk_is_error(ctx, -1)) {
			duk_get_prop_string(ctx, -1, "stack");
			lasterror = duk_safe_to_string(ctx, -1);
			duk_pop(ctx);
		} else {
			lasterror = duk_safe_to_string(ctx, -1);
		}
		log->error("JS error: {}", lasterror);
		settings::setJSConfig(channel_id, "last_error", CleanErrorMessage(lasterror));
		duk_destroy_heap(ctx);
		return false;
	} else {
		settings::setJSConfig(channel_id, "last_error", "");
	}
	if (!exited) {
		duk_pop(ctx);
	}
	duk_destroy_heap(ctx);
	return true;
}

void sandbox_fatal(void *udata, const char *msg) {
	// Yeah, according to the docs a fatal can never return. Technically, it doesnt.
	// At this point we should probably destroy the duk context as bad.
	std::string error = msg;
	total_allocated[current_context] = 0;
	throw std::runtime_error("JS error: " + error);
}

void sandbox_free(void *udata, void *ptr) {
	alloc_hdr *hdr;

	if (!ptr) {
		return;
	}
	hdr = (alloc_hdr *) (((char *) ptr) - sizeof(alloc_hdr));
	auto iter = total_allocated.find(current_context);
	iter->second -= hdr->u.sz;
	free((void *) hdr);
}

void *sandbox_alloc(void *udata, duk_size_t size) {
	alloc_hdr *hdr;

	if (size == 0) {
		return NULL;
	}

	auto iter = total_allocated.find(current_context);

	if (iter->second + size > max_allocated) {
		c_apis_suck->error("Sandbox maximum allocation size reached, {} requested in sandbox_alloc", (long) size);
		return NULL;
	}

	hdr = (alloc_hdr *) malloc(size + sizeof(alloc_hdr));
	if (!hdr) {
		return NULL;
	}
	hdr->u.sz = size;
	iter->second += size;
	return (void *) (hdr + 1);
}

static void *sandbox_realloc(void *udata, void *ptr, duk_size_t size) {
	alloc_hdr *hdr;

	auto iter = total_allocated.find(current_context);

	/* Handle the ptr-NULL vs. size-zero cases explicitly to minimize
	 * platform assumptions.  You can get away with much less in specific
	 * well-behaving environments.
	 */

	if (ptr) {
		hdr = (alloc_hdr *) (((char *) ptr) - sizeof(alloc_hdr));
		size_t old_size = hdr->u.sz;

		if (size == 0) {
			iter->second -= old_size;
			free((void *) hdr);
			return NULL;
		} else {
			if (iter->second - old_size + size > max_allocated) {
				c_apis_suck->error("Sandbox maximum allocation size reached, {} requested in sandbox_realloc", (long) size);
				return NULL;
			}

			void* t = realloc((void *) hdr, size + sizeof(alloc_hdr));

			if (!t) {
				return NULL;
			}
			hdr = (alloc_hdr *) t;
			iter->second -= old_size;
			iter->second += size;
			hdr->u.sz = size;
			return (void *) (hdr + 1);
		}
	} else {
		if (size == 0) {
			return NULL;
		} else {
			if (iter->second + size > max_allocated) {
				c_apis_suck->error("Sandbox maximum allocation size reached, {} requested in sandbox_realloc", (long) size);
				return NULL;
			}

			hdr = (alloc_hdr *) malloc(size + sizeof(alloc_hdr));
			if (!hdr) {
				return NULL;
			}
			hdr->u.sz = size;
			iter->second += size;
			return (void *) (hdr + 1);
		}
	}
}

JSModule::JSModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml)
{
	ml->Attach({ I_OnMessage }, this);
	js = new JS(bot->core.log, bot);
}

JSModule::~JSModule()
{
	delete js;
}

std::string JSModule::GetVersion()
{
	/* NOTE: This version string below is modified by a pre-commit hook on the git repository */
	std::string version = "$ModVer 18$";
	return "1.0." + version.substr(8,version.length() - 9);
}

std::string JSModule::GetDescription()
{
	return "JavaScript Per-Channel Custom Events";
}

bool JSModule::OnMessage(const modevent::message_create &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions)
{
	std::unordered_map<std::string, json> jsonstore;
	modevent::message_create msg = message;

	if (js->channelHasJS(msg.channel.get_id().get())) {

		json chan;
		const aegis::channel& c = message.channel;
		chan["name"] = c.get_name();
		chan["nsfw"] = c.nsfw();
		chan["dm"] = c.is_dm();
		json guild;
		aegis::guild* g = bot->core.find_guild(message.msg.get_guild_id());
		if (g) {
			guild["name"] = g->get_name();
			guild["owner"] = std::to_string(g->get_owner());
			guild["id"] = std::to_string(message.msg.get_guild_id());
			guild["region"] = g->get_region();
			guild["member_count"] = g->get_member_count();
			jsonstore["guild"] = guild;
		}
		jsonstore["channel"] = chan;
		aegis::gateway::objects::to_json(jsonstore["message"], message.msg);
		aegis::gateway::objects::to_json(jsonstore["author"], message.msg.author);
		jsonstore["message"]["id"] = std::to_string(message.msg.get_id());
		jsonstore["message"]["nonce"] = std::to_string(message.msg.nonce);
		jsonstore["mentions"] = stringmentions;
		jsonstore["channel"]["id"] = std::to_string(c.get_id());
		jsonstore["channel"]["guild_id"] = std::to_string(message.msg.get_guild_id().get());
		jsonstore["author"]["id"] = std::to_string(message.msg.author.id);
		jsonstore["author"]["guild_id"] = jsonstore["channel"]["guild_id"];

		js->run(c.get_id().get(), jsonstore);
		return !js->hasReplied();
	}
	return true;
}

ENTRYPOINT(JSModule);

