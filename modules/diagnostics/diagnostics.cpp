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

#include <dpp/dpp.h>
#include <fmt/format.h>
#include <sporks/bot.h>
#include <sporks/regex.h>
#include <sporks/modules.h>
#include <sporks/stringops.h>
#include <sporks/database.h>
#include <sstream>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

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
	return ram * 1024;
}


std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		return "";
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

/**
 * Provides diagnostic commands for monitoring the bot and debugging it interactively while it's running.
 */

class DiagnosticsModule : public Module
{
	PCRE* diagnosticmessage;
	double microseconds_ping;
public:
	DiagnosticsModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml)
	{
		ml->Attach({ I_OnMessage, I_OnRestEnd }, this);
		diagnosticmessage = new PCRE("^sudo(\\s+(.+?))$", true);
	}

	virtual ~DiagnosticsModule()
	{
		delete diagnosticmessage;
	}

	virtual std::string GetVersion()
	{
		/* NOTE: This version string below is modified by a pre-commit hook on the git repository */
		std::string version = "$ModVer 30$";
		return "1.0." + version.substr(8,version.length() - 9);
	}

	virtual std::string GetDescription()
	{
		return "Diagnostic Commands (sudo), '@Sporks sudo'";
	}

	virtual bool OnRestEnd(std::chrono::steady_clock::time_point start_time, uint16_t code)
	{
		microseconds_ping = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time).count();
		return true;
	}

	virtual bool OnMessage(const dpp::message_create_t &message, const std::string& clean_message, bool mentioned, const std::vector<std::string> &stringmentions)
	{
		std::vector<std::string> param;

		if (mentioned && diagnosticmessage->Match(clean_message, param) && param.size() >= 3) {

			dpp::message msg = *(message.msg);
			std::stringstream tokens(trim(param[2]));
			std::string subcommand;
			tokens >> subcommand;

			bot->core->log(dpp::ll_info, fmt::format("SUDO: <{}> {}", msg.author ? msg.author->username : "", clean_message));

			/* Get owner snowflake id from config file */
			dpp::snowflake owner_id = from_string<int64_t>(Bot::GetConfig("owner"), std::dec);

			/* Only allow these commands to the bot owner */
			if (msg.author && msg.author->id == owner_id) {

				if (param.size() < 3) {
					/* Invalid number of parameters */
					EmbedSimple("Sudo make me a sandwich.", msg.channel_id);
				} else {
					/* Module list command */
					if (lowercase(subcommand) == "modules") {
						std::stringstream s;

						// NOTE: GetModuleList's reference is safe from within a module event
						const ModMap& modlist = bot->Loader->GetModuleList();

						s << "```diff" << std::endl;
						s << fmt::format("- ╭─────────────────────────┬───────────┬────────────────────────────────────────────────╮") << std::endl;
						s << fmt::format("- │ Filename                | Version   | Description                                    |") << std::endl;
						s << fmt::format("- ├─────────────────────────┼───────────┼────────────────────────────────────────────────┤") << std::endl;

						for (auto mod = modlist.begin(); mod != modlist.end(); ++mod) {
							s << fmt::format("+ │ {:23} | {:9} | {:46} |", mod->first, mod->second->GetVersion(), mod->second->GetDescription()) << std::endl;
						}
						s << fmt::format("+ ╰─────────────────────────┴───────────┴────────────────────────────────────────────────╯") << std::endl;
						s << "```";

						dpp::channel* c = dpp::find_channel(msg.channel_id);
						if (c) {
							if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == c->guild_id) {
								bot->core->message_create(dpp::message(c->id, s.str()));
								bot->sent_messages++;
							}
						}
						
					} else if (lowercase(subcommand) == "load") {
						/* Load a module */
						std::string modfile;
						tokens >> modfile;
						if (bot->Loader->Load(modfile)) {
							EmbedSimple("Loaded module: " + modfile, msg.channel_id);
						} else {
							EmbedSimple(std::string("Can't do that: ``") + bot->Loader->GetLastError() + "``", msg.channel_id);
						}
					} else if (lowercase(subcommand) == "unload") {
						/* Unload a module */
						std::string modfile;
						tokens >> modfile;
						if (modfile == "module_diagnostics.so") {
							EmbedSimple("I suppose you think that's funny, dont you? *I'm sorry. can't do that, dave.*", msg.channel_id);
						} else {
							if (bot->Loader->Unload(modfile)) {
								EmbedSimple("Unloaded module: " + modfile, msg.channel_id);
							} else {
								EmbedSimple(std::string("Can't do that: ``") + bot->Loader->GetLastError() + "``", msg.channel_id);
							}
						}
					} else if (lowercase(subcommand) == "reload") {
						/* Reload a currently loaded module */
						std::string modfile;
						tokens >> modfile;
						if (modfile == "module_diagnostics.so") {
							EmbedSimple("I suppose you think that's funny, dont you? *I'm sorry. can't do that, dave.*", msg.channel_id);
						} else {
							if (bot->Loader->Reload(modfile)) {
								EmbedSimple("Reloaded module: " + modfile, msg.channel_id);
							} else {
								EmbedSimple(std::string("Can't do that: ``") + bot->Loader->GetLastError() + "``", msg.channel_id);
							}
						}
					} else if (lowercase(subcommand) == "threadstats") {
						std::string result = exec("top -b -n1 -d0 | head -n7 && top -b -n1 -d0 -H | grep \"./bot\\|run.sh\" | grep -v grep | grep -v perl");
						dpp::channel* c = dpp::find_channel(msg.channel_id);
						if (c) {
							if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == c->guild_id) {
								bot->core->message_create(dpp::message(msg.channel_id, "```" + result + "```"));
								bot->sent_messages++;
							}
						}
					} else if (lowercase(subcommand) == "lock") {
						std::string keyword;
						std::getline(tokens, keyword);
						keyword = trim(keyword);
						db::query("UPDATE infobot SET locked = 1 WHERE key_word = '?'", {keyword});
						EmbedSimple("**Locked** key word: " + keyword, msg.channel_id);
					} else if (lowercase(subcommand) == "unlock") {
						std::string keyword;
						std::getline(tokens, keyword);
						keyword = trim(keyword);
						db::query("UPDATE infobot SET locked = 0 WHERE key_word = '?'", {keyword});
						EmbedSimple("**Unlocked** key word: " + keyword, msg.channel_id);
					} else if (lowercase(subcommand) == "sql") {
						std::string sql;
						std::getline(tokens, sql);
						sql = trim(sql);
						db::resultset rs = db::query(sql, {});
						std::stringstream w;
						if (rs.size() == 0) {
							if (db::error() != "") {
								EmbedSimple("SQL Error: " + db::error(), msg.channel_id);
							} else {
								EmbedSimple("Successfully executed, no rows returned.", msg.channel_id);
							}
						} else {
							w << "- " << sql << std::endl;
							auto check = rs[0].begin();
							w << "+ Rows Returned: " << rs.size() << std::endl;
							for (auto name = rs[0].begin(); name != rs[0].end(); ++name) {
								if (name == rs[0].begin()) {
									w << "  ╭";
								}
								w << "────────────────────";
								check = name;
								w << (++check != rs[0].end() ? "┬" : "╮\n");
							}
							w << "  ";
							for (auto name = rs[0].begin(); name != rs[0].end(); ++name) {
								w << fmt::format("│{:20}", name->first.substr(0, 20));
							}
							w << "│" << std::endl;
							for (auto name = rs[0].begin(); name != rs[0].end(); ++name) {
								if (name == rs[0].begin()) {
									w << "  ├";
								}
								w << "────────────────────";
								check = name;
								w << (++check != rs[0].end() ? "┼" : "┤\n");
							}
							for (auto row : rs) {
								if (w.str().length() < 1900) {
									w << "  ";
									for (auto field : row) {
										w << fmt::format("│{:20}", field.second.substr(0, 20));
									}
									w << "│" << std::endl;
								}
							}
							for (auto name = rs[0].begin(); name != rs[0].end(); ++name) {
								if (name == rs[0].begin()) {
									w << "  ╰";
								}
								w << "────────────────────";
								check = name;
								w << (++check != rs[0].end() ? "┴" : "╯\n");
							}
							dpp::channel* c = dpp::find_channel(msg.channel_id);
							if (c) {
								if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == c->guild_id) {
									bot->core->message_create(dpp::message(msg.channel_id, "```diff\n" + w.str() + "```"));
									bot->sent_messages++;
								}
							}
						}
					} else if (lowercase(subcommand) == "restart") {
						EmbedSimple("Restarting...", msg.channel_id);
						::sleep(5);
						/* Note: exit here will restart, because we run the bot via run.sh which restarts the bot on quit. */
						exit(0);
					} else if (lowercase(subcommand) == "ping") {
						dpp::channel* c = dpp::find_channel(msg.channel_id);
						if (c) {
							std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
							dpp::snowflake cid = msg.channel_id;
							bot->core->message_create(dpp::message(msg.channel_id, "Pinging..."), [cid, this, start_time](const dpp::confirmation_callback_t & state) {
								double microseconds_ping = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time).count();
								dpp::snowflake mid = (std::get<dpp::message>(state.value)).id;
								this->bot->core->message_delete(mid, cid);
								this->EmbedSimple(fmt::format("**Pong!** REST Response time: {:.3f} ms", microseconds_ping / 1000, 4), cid);
							});
						}
					} else if (lowercase(subcommand) == "lookup") {
						int64_t gnum = 0;
						tokens >> gnum;
						dpp::guild* guild = dpp::find_guild(gnum);
						if (guild) {
							EmbedSimple(fmt::format("**Guild** {} is on **shard** #{}", gnum, 0), msg.channel_id);
						} else {
							EmbedSimple(fmt::format("**Guild** {} is not in my list!", gnum), msg.channel_id);
						}
					} else if (lowercase(subcommand) == "shardstats") {
						std::stringstream w;
						w << "```diff\n";

						uint64_t count = 0, u_count = 0;
						auto& shards = bot->core->get_shards();
						for (auto i = shards.begin(); i != shards.end(); ++i) {
							dpp::DiscordClient* shard = i->second;
							count += shard->GetBytesIn() + shard->GetBytesOut();
							u_count += shard->GetDecompressedBytesIn() + shard->GetBytesOut();
						}

						w << fmt::format("  Total transfer: {} (U: {} | {:.2f}%) Memory usage: {}\n", dpp::utility::bytes(count), dpp::utility::bytes(u_count), (count / (double)u_count)*100, dpp::utility::bytes(GetRSS()));
						w << fmt::format("- ╭──────┬──────────┬───────┬───────┬────────────────┬───────────┬──────────╮\n");
						w << fmt::format("- │shard#│  sequence│servers│members│uptime          │transferred│reconnects│\n");
						w << fmt::format("- ├──────┼──────────┼───────┼───────┼────────────────┼───────────┼──────────┤\n");

						for (auto i = shards.begin(); i != shards.end(); ++i)
						{
							dpp::DiscordClient* s = i->second;
							if (s->IsConnected())
								w << "+ ";
							else
								w << "  ";
							w << fmt::format("|{:6}|{:10}|{:7}|{:7}|{:>16}|{:>11}|{:10}|",
											 s->shard_id,
											 s->last_seq,
											 s->GetGuildCount(),
											 s->GetMemberCount(),
											 s->Uptime().to_string(),
											 dpp::utility::bytes(s->GetBytesIn() + s->GetBytesOut()),
											 0);
							if (message.from->shard_id == s->shard_id) {
								w << " *\n";
							} else {
								w << "\n";
							}
						}
						w << fmt::format("+ ╰──────┴──────────┴───────┴───────┴────────────────┴───────────┴──────────╯\n");
						w << "```";
						dpp::channel *channel = dpp::find_channel(msg.channel_id);
						if (channel) {
							if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == channel->guild_id) {
								bot->core->message_create(dpp::message(channel->id, w.str()));
								bot->sent_messages++;
							}
						}
					} else {
						/* Invalid command */
						EmbedSimple("Sudo **what**? I don't know what that command means.", msg.channel_id);
					}
				}
			} else {
				/* Access denied */
				EmbedSimple("Make your own sandwich, mortal.", msg.channel_id);
			}

			/* Eat the event */
			return false;
		}
		return true;
	}
};

ENTRYPOINT(DiagnosticsModule);

