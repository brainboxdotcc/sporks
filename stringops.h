#pragma once

#include <string>

std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);

// trim from end of string (right)
inline std::string rtrim(std::string s)
{
	s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
	return s;
}

// trim from beginning of string (left)
inline std::string ltrim(std::string s)
{
	s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
	return s;
}

// trim from both ends of string (right then left)
inline std::string trim(std::string s)
{
	return ltrim(rtrim(s));
}
