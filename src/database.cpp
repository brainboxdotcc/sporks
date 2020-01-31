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

#include <sporks/database.h>
#include <mysql/mysql.h>
#include <iostream>
#include <mutex>

namespace db {

	MYSQL connection;
	std::mutex db_mutex;
	std::map<std::string, MYSQL_STMT*> prepared_cache;

	/**
	 * Connect to mysql database, returns false if there was an error.
	 */
	bool connect(const std::string &host, const std::string &user, const std::string &pass, const std::string &db, int port) {
		std::lock_guard<std::mutex> db_lock(db_mutex);
		if (mysql_init(&connection) != nullptr) {
			my_bool reconnect = 1;
			if (mysql_options(&connection, MYSQL_OPT_RECONNECT, &reconnect) == 0) {
				return mysql_real_connect(&connection, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), port, NULL, CLIENT_MULTI_RESULTS | CLIENT_MULTI_STATEMENTS);
			} else {
				return false;
			}
		} else {
			return false;
		}
	}

	/**
	 * Disconnect from mysql database, for now always returns true.
	 * If there's an error, there isn't much we can do about it anyway.
	 */
	bool close() {
		std::lock_guard<std::mutex> db_lock(db_mutex);
		mysql_close(&connection);
		return true;
	}

	/**
	 * Run a mysql query, with automatic escaping of parameters to prevent SQL injection.
	 * The parameters given should be a vector of strings. You can instantiate this using "{}".
	 * For example: db::query("UPDATE foo SET bar = '?' WHERE id = '?'", {"baz", "3"});
	 * Returns a resultset of the results as rows. Avoid returning massive resultsets if you can.
	 */
	resultset query(const std::string &format, std::vector<std::string> parameters) {

		/**
		 * One DB handle can't query the database from multiple threads at the same time.
		 * To prevent corruption of results, put a lock guard on queries.
		 */
		std::lock_guard<std::mutex> db_lock(db_mutex);

		resultset rv;

		if (prepared_cache.find(format) == prepared_cache.end()) {
			MYSQL_STMT* p_statement = mysql_stmt_init(&connection);
			if (p_statement) {
				mysql_stmt_prepare(p_statement, format.c_str(), format.length());
				prepared_cache[format] = p_statement;
			}
		}
		auto statement_iter = prepared_cache.find(format)->second;

		MYSQL_BIND bound_parameters[parameters.size()];

		for (size_t p = 0; p < parameters.size(); ++p) {
			bind[p].buffer_type = MYSQL_TYPE_VARCHAR;
			bind[p].buffer = parameters[p].c_str();
		}
		mysql_stmt_bind_param(stmt, bind);


		/**
		 * Escape all parameters properly
		 */
		for (unsigned int i = 0; i < parameters.size(); ++i) {
			/* Worst case scenario: Every character becomes two, plus NULL terminator*/
			char out[parameters[i].length() * 2 + 1];
			/* Some moron thought it was a great idea for mysql_real_escape_string to return an unsigned but use -1 to indicate error.
			 * This stupid cast below is the actual recommended error check from the reference manual. Seriously stupud.
			 */
			if (mysql_real_escape_string(&connection, out, parameters[i].c_str(), parameters[i].length()) == (unsigned long)-1) {
				/* At this point 'rv' always contains an empty set. Really, we should never see this error anyway but
				 * it's always better to error check everything.
				 */
				return rv;
			}
			parameters[i] = out;
		}

		unsigned int param = 0;
		std::string querystring;

		/**
		 * Search and replace escaped parameters in the query string.
		 *
		 * TODO: Really, I should use a cached query and the built in parameterisation for this.
		 *       It would scale a lot better.
		 */
		for (auto v = format.begin(); v != format.end(); ++v) {
			if (*v == '?') {
				querystring.append(parameters[param]);
				if (param != parameters.size() - 1) {
					param++;
				}
			} else {
				querystring += *v;
			}
		}

		int result = mysql_query(&connection, querystring.c_str());

		/**
		 * On successful query collate results into a std::map
		 */
		if (result == 0) {
			MYSQL_RES *a_res = mysql_use_result(&connection);
			if (a_res) {
				MYSQL_ROW a_row;
				while ((a_row = mysql_fetch_row(a_res))) {
					MYSQL_FIELD *fields = mysql_fetch_fields(a_res);
					row thisrow;
					unsigned int field_count = 0;
					if (mysql_num_fields(a_res) == 0) {
						break;
					}
					if (fields && mysql_num_fields(a_res)) {
						while (field_count < mysql_num_fields(a_res)) {
							std::string a = (fields[field_count].name ? fields[field_count].name : "");
							std::string b = (a_row[field_count] ? a_row[field_count] : "");
							thisrow[a] = b;
							field_count++;
						}
						rv.push_back(thisrow);
					}
				}
				mysql_free_result(a_res);
			}
		} else {
			/**
			 * In properly written code, this should never happen. Famous last words.
			 */
			std::cout << "SQL error: " << mysql_errno(&connection) << " on query: " << querystring << std::endl;
		}
		return rv;
	}
};
