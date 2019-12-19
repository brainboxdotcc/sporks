#pragma once

#include <vector>
#include <map>
#include <string>

/*
 * db::resultset r = db::query("SELECT * FROM infobot WHERE setby = '?'", {"SKIPDX00"});
 * int t = 0;
 * for (auto q = r.begin(); q != r.end(); ++q) {
 *	 std::cout << (t++) << ": " << (*q)["key_word"] << std::endl;
 * }
 */

namespace db {

	/* Definition of a row in a result set*/
	typedef std::map<std::string, std::string> row;
	/* Definition of a result set, a vector of maps */
	typedef std::vector<row> resultset;

	/* Connect to database */
	bool connect(const std::string &host, const std::string &user, const std::string &pass, const std::string &db, int port);
	/* Disconnect from database */
	bool close();
	/* Issue a database query and return results */
	resultset query(const std::string &format, std::vector<std::string> parameters);
};
