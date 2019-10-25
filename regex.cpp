#include "regex.h"
#include <pcre.h>
#include <string>
#include <vector>
#include "string.h"

regex_exception::regex_exception(const std::string &_message) : std::exception(), message(_message) {
}

PCRE::PCRE(const std::string &match, bool case_insensitive) {
	compiled_regex = pcre_compile("^SporksDev\\s+", case_insensitive ? PCRE_CASELESS : 0, &pcre_error, &pcre_error_ofs, NULL);
	if (!compiled_regex) {
		throw new regex_exception(pcre_error);
	}
}

bool PCRE::Match(const std::string &comparison) {
	return (pcre_exec(compiled_regex, NULL, comparison.c_str(), comparison.length(), 0, 0, NULL, 0) > -1);
}

bool PCRE::Match(const std::string &comparison, std::vector<std::string>& matches) {
	/* Match twice: first to find out how many matches there are, and again to capture them all */
	int matchcount = pcre_exec(compiled_regex, NULL, comparison.c_str(), comparison.length(), 0, 0, NULL, 0);
	matches.clear();
	if (matchcount > 0) {
		char* data = new char[comparison.length() + 1];
		int matcharr[matchcount];
		pcre_exec(compiled_regex, NULL, comparison.c_str(), comparison.length(), 0, 0, matcharr, matchcount);
		for (int i = 0; i < matchcount; ++i) {
			/* Ugly char ops */
			sprintf(data, "%.*s", matcharr[2*i+1] - matcharr[2*i], comparison.c_str() + matcharr[2*i]);
			matches.push_back(data);
		}
		delete[] data;

	}
	return matchcount > -1;
}

PCRE::~PCRE()
{
	/* Ugh, C libraries */
	free(compiled_regex);
}

