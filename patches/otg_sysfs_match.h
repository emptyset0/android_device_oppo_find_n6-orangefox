#pragma once
#include <fnmatch.h>
#include <string>

// Match a complete configured sysfs ancestor, not the prefix before the first '*'.
// FNM_PATHNAME prevents a wildcard from spanning multiple sysfs components.
inline bool RecoverySysfsMatch(const std::string& pattern, std::string path) {
    if (path.compare(0, 9, "/devices/") != 0 ||
        pattern.compare(0, 9, "/devices/") != 0)
        return false;
    for (;;) {
        if (fnmatch(pattern.c_str(), path.c_str(), FNM_PATHNAME) == 0)
            return true;
        auto slash = path.rfind('/');
        if (slash <= 8 || slash == std::string::npos)
            return false;
        path.resize(slash);
    }
}
