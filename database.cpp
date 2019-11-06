#include "database.h"
#include <mysql/mysql.h>
#include <iostream>
#include <mutex>

namespace db {

	MYSQL connection;
	std::mutex db_mutex;

	bool connect(const std::string &host, const std::string &user, const std::string &pass, const std::string &db, int port) {
		mysql_init(&connection);
		return mysql_real_connect(&connection, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), port, NULL, CLIENT_MULTI_RESULTS | CLIENT_MULTI_STATEMENTS);
	}

	bool close() {
		mysql_close(&connection);
		return true;
	}

	resultset query(const std::string &format, std::vector<std::string> parameters) {

		std::lock_guard<std::mutex> db_lock(db_mutex);

		resultset rv;

		for (unsigned int i = 0; i < parameters.size(); ++i) {
			char out[parameters[i].length() + 1];
			mysql_real_escape_string(&connection, out, parameters[i].c_str(), parameters[i].length());
			parameters[i] = out;
		}

		unsigned int param = 0;
		std::string querystring;

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
		//std::cout << "SQL: " << querystring << std::endl;

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
			std::cout << "SQL error: " << mysql_errno(&connection) << " on query: " << querystring << std::endl;
		}
		return rv;
	}
};
