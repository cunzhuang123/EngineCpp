

// Main.cpp
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN   // 可选：减少 Windows 头体积
#include <windows.h>

#include <iostream>
#include <fstream>           // 用于文件操作

#include "TrackUtils.h"
#include "Engine.h"
#include "src/ScopedProfiler.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;


const int WIDTH = 1920;
const int HEIGHT = 1080;


int main() {
    std::cerr << "main enter" << std::endl;
    // 设置控制台输出编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    // 设置控制台输入编码为 UTF-8（可选，视需求而定）
    SetConsoleCP(CP_UTF8);    

    try {
        ScopedProfiler profiler("main");

        json tracksJson;
        bool isDebugger = IsDebuggerPresent();
        if (isDebugger) {
            #ifndef PROJECT_ROOT_DIR
            #define PROJECT_ROOT_DIR ".."
            #endif
            std::string jsonPath = PROJECT_ROOT_DIR;
            jsonPath += "/test/track.json";
            std::ifstream input_file(jsonPath);
            if (!input_file.is_open()) {
                json errorJson;
                errorJson["error"] = "无法打开文件 track.json";
                std::cerr << errorJson.dump() << std::endl;
                return 1;
            }
            input_file >> tracksJson;
        }
        else
        {
            // 从标准输入读取 JSON 字符串
            std::string jsonString;
            std::getline(std::cin, jsonString);

            if (jsonString.empty()) {
                json errorJson;
                errorJson["error"] = "未收到 JSON 输入";
                std::cerr << errorJson.dump() << std::endl;
                return 1;
            }
            // 解析 JSON 字符串
            tracksJson = json::parse(jsonString);
        }

        Engine engine;
        engine.Init(tracksJson["width"], tracksJson["height"], tracksJson["isDebug"]);  // 替换为实际的 WIDTH 和 HEIGHT
        engine.UpdateTracks(tracksJson);
        
        // 执行播放，并获取结果
        engine.Play(tracksJson["startTime"], tracksJson["endTime"], tracksJson["stepTime"], tracksJson["isDebug"], tracksJson["outputPath"], tracksJson["fps"], tracksJson["mBitRate"]);
        json resultJson;
        resultJson["result"] = "处理成功";
        // 输出结果 JSON
        std::cout << resultJson.dump() << std::endl;

        return 0;
    }
    catch (json::parse_error& e) {
        json errorJson;
        errorJson["error"] = "解析错误: " + std::string(e.what());
        std::cerr << errorJson.dump() << std::endl;
        return 1;
    }
    catch (json::type_error& e) {
        json errorJson;
        errorJson["error"] = "类型错误: " + std::string(e.what());
        std::cerr << errorJson.dump() << std::endl;
        return 1;
    }
    catch (std::exception& e) {
        json errorJson;
        errorJson["error"] = "未知错误: " + std::string(e.what());
        std::cerr << errorJson.dump() << std::endl;
        return 1;
    }
}