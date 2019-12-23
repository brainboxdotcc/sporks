#include <sporks/stringops.h>
#include <locale>
#include <iostream>
#include <algorithm>

/**
 * Search and replace a string within another string, case sensitive.
 */
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace) {
	size_t pos = 0;

	std::string subject_lc = lowercase(subject);
	std::string search_lc = lowercase(search);
	std::string replace_lc = lowercase(replace);

	while((pos = subject_lc.find(search_lc, pos)) != std::string::npos) {

		 subject.replace(pos, search.length(), replace);
		 subject_lc.replace(pos, search_lc.length(), replace_lc);

		 pos += replace.length();
	}
	return subject;
}

