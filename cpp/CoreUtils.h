#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include "nlohmann/json.hpp"
#include "src/VideoRenderer.h"

class CoreUtils {
public:
    static std::string trim(const std::string &s);
    static std::vector<std::string> split(const std::string& s, const std::string& delimiters);
    static bool endsWith(const std::string& str, const std::string& suffix);
    static std::optional<std::array<double, 4>> convertHexToColorArray(const std::string& colorStr);
    static std::string convertColorArrayToHex(const std::vector<double>& colorArray);
};

#endif // CORE_UTILS_H