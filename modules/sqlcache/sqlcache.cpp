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
#include <dpp/nlohmann/json.hpp>
#include <fmt/format.h>
#include <sporks/bot.h>
#include <sporks/modules.h>
#include <sporks/stringops.h>
#include <sporks/database.h>
#include <sporks/config.h>
#include <sstream>
#include <thread>

/**
 * Provides caching of users, guilds and memberships to an sql database for use by external programs.
 */

class SQLCacheModule : public Module
{
	/* Queue processing threads */
	std::thread* thr_userqueue;
	std::thread* thr_guildqueue;

	/* Safety mutexes */
	std::mutex user_cache_mutex;
	std::mutex guild_cache_mutex;

	/* True if the thread is to terminate */
	bool terminate;

	/* Userqueue: a queue of users waiting to be written to SQL for the dashboard */
	std::queue<dpp::user> userqueue;
	std::queue<dpp::guild> guildqueue;
public:

	void SaveCachedUsersThread() {
		time_t last_message = time(NULL);
		dpp::user u;
		while (!this->terminate) {
			if (!userqueue.empty()) {
				{
					std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
					u = userqueue.front();
					userqueue.pop();
					bot->counters["userqueue"] = userqueue.size();
				};
				std::string bot = u.is_bot() ? "1" : "0";
				db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {u.id, u.username, u.discriminator, u.avatar.to_string(), bot, u.username, u.discriminator, u.avatar.to_string()});
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			} else {
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			if (time(NULL) > last_message) {
				if (userqueue.size() > 0) {
					bot->core->log(dpp::ll_info, fmt::format("User queue size: {} objects", userqueue.size()));
				}
				last_message = time(NULL) + 60;
			}
		}
	}

	void SaveCachedGuildsThread() {
		time_t last_message = time(NULL);
		dpp::guild gc;
		while (!this->terminate) {
			if (!guildqueue.empty()) {
				{
					std::lock_guard<std::mutex> user_cache_lock(guild_cache_mutex);
					gc = guildqueue.front();
					guildqueue.pop();
					bot->counters["guildqueue"] = guildqueue.size();
				};
				for (auto i = gc.channels.begin(); i != gc.channels.end(); ++i) {
					getSettings(bot, *i, gc.id);
				}
				for (auto i = gc.members.begin(); i != gc.members.end(); ++i) {
					dpp::user* u = dpp::find_user(i->second.user_id);
					if (!u)
						continue;
					std::lock_guard<std::mutex> user_cache_lock(user_cache_mutex);
					userqueue.push(*u);
					bot->counters["userqueue"] = userqueue.size();
					std::string roles_str;
					for (auto n = i->second.get_roles().begin(); n != i->second.get_roles().end(); ++n) {
						roles_str.append(std::to_string(*n)).append(",");
					}
					roles_str = roles_str.substr(0, roles_str.length() - 1);
					std::string dashboard = "0";
					if (gc.owner_id == i->second.user_id) {
						/* Server owner */
						dashboard = "1";
					}
					db::query("INSERT INTO infobot_membership (member_id, guild_id, nick, roles, dashboard) VALUES(?, ?, '?', '?','?') ON DUPLICATE KEY UPDATE nick = '?', roles = '?', dashboard = '?'", {i->second.user_id, gc.id, i->second.get_nickname(), roles_str, dashboard, i->second.get_nickname(), roles_str, dashboard});
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			} else {
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			if (time(NULL) > last_message) {
				if (guildqueue.size() > 0) {
					bot->core->log(dpp::ll_info, fmt::format("User guild size: {} objects", guildqueue.size()));
				}
				last_message = time(NULL) + 60;
			}
		}
	}

	SQLCacheModule(Bot* instigator, ModuleLoader* ml) : Module(instigator, ml), thr_userqueue(nullptr), thr_guildqueue(nullptr), terminate(false)
	{
		ml->Attach({ I_OnGuildCreate, I_OnPresenceUpdate, I_OnGuildMemberAdd, I_OnChannelCreate, I_OnChannelDelete, I_OnGuildDelete, I_OnGuildMemberRemove }, this);
		bot->counters["userqueue"] = 0;
		thr_userqueue = new std::thread(&SQLCacheModule::SaveCachedUsersThread, this);
		thr_guildqueue = new std::thread(&SQLCacheModule::SaveCachedGuildsThread, this);
	}

	virtual ~SQLCacheModule()
	{
		terminate = true;
		bot->DisposeThread(thr_userqueue);
		bot->DisposeThread(thr_guildqueue);
		bot->counters["userqueue"] = 0;
		bot->counters["guildqueue"] = 0;
	}

	virtual std::string GetVersion()
	{
		/* NOTE: This version string below is modified by a pre-commit hook on the git repository */
		std::string version = "$ModVer 13$";
		return "1.0." + version.substr(8,version.length() - 9);
	}

	virtual std::string GetDescription()
	{
		return "User/Guild SQL Cache";
	}

	virtual bool OnGuildCreate(const dpp::guild_create_t &gc)
	{
		db::query("INSERT INTO infobot_shard_map (guild_id, shard_id, name, icon, unavailable, owner_id) VALUES('?','?','?','?','?','?') ON DUPLICATE KEY UPDATE shard_id = '?', name = '?', icon = '?', unavailable = '?', owner_id = '?'",
			{
				gc.created->id,
				(gc.created->id >> 22) % bot->core->get_shards().size(),
				gc.created->name,
				gc.created->icon.as_iconhash().to_string(),
				gc.created->is_unavailable(),
				gc.created->owner_id,
				(gc.created->id >> 22) % bot->core->get_shards().size(),
				gc.created->name,
				gc.created->icon.as_iconhash().to_string(),
				gc.created->is_unavailable(),
				gc.created->owner_id
			}
		);

		{
			std::lock_guard<std::mutex> guild_cache_lock(guild_cache_mutex);
			guildqueue.push(*(gc.created));
			bot->counters["guildqueue"] = guildqueue.size();
		}

		return true;
	}

	virtual bool OnPresenceUpdate()
	{
		auto& shards = bot->core->get_shards();
		for (auto i = shards.begin(); i != shards.end(); ++i) {
			dpp::discord_client* shard = i->second;
			dpp::utility::uptime up = shard->get_uptime();
			uint64_t uptime = up.secs = (up.mins / 60) + (up.hours / 60 / 60) + (up.days / 60 / 60 / 24);
			db::query("INSERT INTO infobot_shard_status (id, connected, online, uptime, transfer, transfer_compressed) VALUES('?','?','?','?','?','?') ON DUPLICATE KEY UPDATE connected = '?', online = '?', uptime = '?', transfer = '?', transfer_compressed = '?'",
				{
					shard->shard_id,
					shard->is_connected(),
					shard->is_connected(),
					uptime,
					shard->get_bytes_in() + shard->get_bytes_out(),
					(int64_t)shard->get_decompressed_bytes_in() + shard->get_bytes_out(),
					shard->is_connected(),
					shard->is_connected(),
					uptime,
					shard->get_bytes_in() + shard->get_bytes_out(),
					(int64_t)shard->get_decompressed_bytes_in() + shard->get_bytes_out()
				}
			);
		}
		return true;
	}

	virtual bool OnGuildMemberRemove(const dpp::guild_member_remove_t &gmr)
	{
		db::query("DELETE FROM infobot_membership WHERE member_id = '?'", {gmr.removed.id});
		return true;
	}

	virtual bool OnGuildMemberAdd(const dpp::guild_member_add_t &gma)
	{
		if (!gma.added.user_id == 0)
			return true;
		dpp::user* u = dpp::find_user(gma.added.user_id);
		if (!u)
			return true;
		bool _bot = u->is_bot();
		std::string roles_str;
		dpp::guild* g = dpp::find_guild(gma.adding_guild->id);
		for (auto n = gma.added.get_roles().begin(); n != gma.added.get_roles().end(); ++n) {
			roles_str.append(std::to_string(*n)).append(",");
		}
		roles_str = roles_str.substr(0, roles_str.length() - 1);
		std::string dashboard = "0";
		if (g->owner_id == gma.added.user_id) {
			/* Server owner */
			dashboard = "1";
		}
		db::query("INSERT INTO infobot_discord_user_cache (id, username, discriminator, avatar, bot) VALUES(?, '?', '?', '?', ?) ON DUPLICATE KEY UPDATE username = '?', discriminator = '?', avatar = '?'", {u->id, u->username, u->discriminator, u->avatar.to_string(), _bot, u->username, u->discriminator, u->avatar.to_string()});
		db::query("INSERT INTO infobot_membership (member_id, guild_id, nick, roles, dashboard) VALUES(?, ?, '?', '?','?') ON DUPLICATE KEY UPDATE nick = '?', roles = '?', dashboard = '?'", {u->id, gma.added.guild_id, gma.added.get_nickname(), roles_str, dashboard, gma.added.get_nickname(), roles_str, dashboard});
		return true;
	}

	virtual bool OnChannelCreate(const dpp::channel_create_t& channel_create)
	{
		getSettings(bot, channel_create.created->id, channel_create.created->guild_id);
		return true;
	}

	virtual bool OnChannelDelete(const dpp::channel_delete_t& cd)
	{
		db::query("DELETE FROM infobot_discord_settings WHERE id = '?'", {cd.deleted.id});
		return true;
	}

	virtual bool OnGuildDelete(const dpp::guild_delete_t& gd)
	{
		db::query("DELETE FROM infobot_discord_settings WHERE guild_id = '?'", {gd.deleted.id});
		db::query("DELETE FROM infobot_shard_map WHERE guild_id = '?'", {gd.deleted.id});
		db::query("DELETE FROM infobot_membership WHERE guild_id = '?'", {gd.deleted.id});
		return true;
	}
};

ENTRYPOINT(SQLCacheModule);

