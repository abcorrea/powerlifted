#ifndef SEARCH_UTILS_STRING_UTILS_H
#define SEARCH_UTILS_STRING_UTILS_H

#include <algorithm>
#include <cctype>
#include <string>

namespace utils {

inline bool iequals(const std::string &a, const std::string &b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(),
                      [](unsigned char ca, unsigned char cb) {
                          return std::tolower(ca) == std::tolower(cb);
                      });
}

}

#endif // SEARCH_UTILS_STRING_UTILS_H
