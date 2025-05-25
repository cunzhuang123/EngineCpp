

#include "CoreUtils.h"
#include <algorithm> // 用于 std::min
#include <iostream>  // 用于调试输出（可选）
#include <regex>
#include <sstream>

// 辅助函数：去掉字符串两端空白字符
// 去除首尾空白字符
std::string CoreUtils::trim(const std::string &s)
{
    const char* ws = " \t\n\r";
    size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}


std::vector<std::string> CoreUtils::split(const std::string& s, const std::string& delimiters) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = s.find_first_of(delimiters);
    
    while (end != std::string::npos) {
        if (end != start) {
            tokens.push_back(s.substr(start, end - start));
        }
        start = end + 1;
        end = s.find_first_of(delimiters, start);
    }
    
    // 添加最后一个部分
    if (start < s.length()) {
        tokens.push_back(s.substr(start));
    }
    
    return tokens;
}

bool CoreUtils::endsWith(const std::string& str, const std::string& suffix)
{
    if (str.size() < suffix.size())
        return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}


std::optional<std::array<double, 4>> CoreUtils::convertHexToColorArray(const std::string& colorStr)
{
    /* 1. 基本格式校验 ------------------------------------------------------- */
    static const std::regex hexRegex{R"(^#[A-Fa-f0-9]{6}([A-Fa-f0-9]{2})?$)"};
    if (!std::regex_match(colorStr, hexRegex))
        return std::nullopt;

    /* 2. 将两个 16 进制字符转换为 [0,255] 的整数 ------------------------------ */
    auto hexPairToInt = [](char hi, char lo) -> int {
        auto toVal = [](char c) -> int {
            return std::isdigit(c) ? c - '0'
                                   : std::tolower(c) - 'a' + 10;
        };
        return (toVal(hi) << 4) | toVal(lo);
    };

    /* 3. 提取并归一化 R、G、B（必有） --------------------------------------- */
    double r = hexPairToInt(colorStr[1], colorStr[2]) / 255.0;
    double g = hexPairToInt(colorStr[3], colorStr[4]) / 255.0;
    double b = hexPairToInt(colorStr[5], colorStr[6]) / 255.0;

    /* 4. Alpha 可选，缺省为 1 ----------------------------------------------- */
    double a = 1.0;
    if (colorStr.length() == 9)                       // "#RRGGBBAA"
        a = hexPairToInt(colorStr[7], colorStr[8]) / 255.0;

    return std::array<double, 4>{r, g, b, a};
}

std::string CoreUtils::convertColorArrayToHex(const std::vector<double>& colorArray) {
    if (colorArray.size() < 3) return "";
    
    std::stringstream ss;
    ss << "#";
    
    for (size_t i = 0; i < 3 && i < colorArray.size(); ++i) {
        int val = static_cast<int>(colorArray[i] * 255);
        ss << std::setw(2) << std::setfill('0') << std::hex << (val & 0xFF);
    }
    
    // 处理透明度
    if (colorArray.size() >= 4) {
        int val = static_cast<int>(colorArray[3] * 255);
        ss << std::setw(2) << std::setfill('0') << std::hex << (val & 0xFF);
    }
    
    return ss.str();
}