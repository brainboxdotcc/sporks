#pragma once
#include <string>
#include <vector>
#include <exception>

/**
 * An exception thrown by a regular expression
 */
class regex_exception : public std::exception {
public:
	std::string message;
	regex_exception(const std::string &_message);
};

/**
 * Class PCRE represents a perl compatible regular expression
 * This is internally managed by libpcre.
 * Regular expressions are compiled in the constructor and matched
 * by the Match() methods.
 */
class PCRE
{
	const char* pcre_error;
	int pcre_error_ofs;
	struct real_pcre* compiled_regex;
 public:
	/* Constructor */
	PCRE(const std::string &match, bool case_insensitive = false);
	~PCRE();
	/* Match methods */
	bool Match(const std::string &comparison);
	bool Match(const std::string &comparison, std::vector<std::string>& matches);
};

