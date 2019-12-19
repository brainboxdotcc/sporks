#include <string>
#include <sporks/statusfield.h>

/**
 * Constructor for statusfield, contains a simple field definition for an embed stored as JSON
 */
statusfield::statusfield(const std::string &a, const std::string &b) : name(a), value(b) {
}

