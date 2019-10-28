#pragma once

#include <string>
#include <iomanip>
#include <locale>
#include <algorithm>

template <typename T> std::basic_string<T> lowercase(const std::basic_string<T>& s)
{
    std::basic_string<T> s2 = s;
    std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
    return std::move(s2);
}

template <typename T> std::basic_string<T> uppercase(const std::basic_string<T>& s)
{
    std::basic_string<T> s2 = s;
    std::transform(s2.begin(), s2.end(), s2.begin(), toupper);
    return std::move(s2);
}

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

template<class T> std::string Comma(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

template <typename T> T from_string(const std::string &s, std::ios_base & (*f)(std::ios_base&))
{
	T t;
	std::istringstream iss(s);
	iss >> f, iss >> t;
	return t;
}
