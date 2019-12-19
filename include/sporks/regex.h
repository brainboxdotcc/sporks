#pragma once
#include <string>
#include <vector>
#include <exception>

class regex_exception : public std::exception {
public:
	std::string message;
	regex_exception(const std::string &_message);
};

class PCRE
{
	const char* pcre_error;
	int pcre_error_ofs;
	struct real_pcre* compiled_regex;
 public:
	PCRE(const std::string &match, bool case_insensitive = false);
	~PCRE();
	bool Match(const std::string &comparison);
	bool Match(const std::string &comparison, std::vector<std::string>& matches);
};
