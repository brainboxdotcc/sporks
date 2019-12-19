#pragma once

#include <string>

/**
 * Represents a simple string name/value pair that can be instantiated at compile time
 * for field lists used with embeds.
 */
struct statusfield {
	/** Name */
	std::string name;
	/** Value, max size 2048 */
	std::string value;
	/** Constructor */
	statusfield(const std::string &a, const std::string &b);
};

