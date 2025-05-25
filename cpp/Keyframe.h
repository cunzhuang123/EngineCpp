// Keyframe.h
#ifndef KEYFRAME_H
#define KEYFRAME_H

// #pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <optional>
#include "nlohmann/json.hpp"

class Engine;  // Forward declaration
class VideoRenderer;  // Forward declaration
class PluginRenderer;

class Keyframe {
public:
    // 获取关键帧插值结果
    static nlohmann::json getKeyframeValue(const nlohmann::json& keyframeArray, double globalTime, Engine& engine);

    // 更新渲染器调整参数（位置、旋转、缩放、透明度）
    static void updateRendererAdjust(VideoRenderer& renderer, double globalTime, const nlohmann::json& sequence, Engine& engine);

    // 更新文本渲染器属性
    static void updateTextRenderer(VideoRenderer& renderer, double globalTime, const nlohmann::json& sequence, Engine& engine);

    static nlohmann::json getKeyframeControl(const nlohmann::json& plugin, double globalTime, Engine& engine);
    // 更新渲染器插件参数
    static void updateRendererPlugin(VideoRenderer& renderer, double globalTime, const nlohmann::json& sequence, Engine& engine, const nlohmann::json& plugin, int pluginIndex);

    // 检查是否是非空普通对象
    static bool isPlainNoEmptyObject(const nlohmann::json& val);

    // 主更新函数
    static void updateRenderer(VideoRenderer& renderer, double globalTime, const nlohmann::json& sequence, Engine& engine);
    static void updatePluginRenderer(PluginRenderer& pluginRenderer, double globalTime, const nlohmann::json& sequence, Engine& engine);

// protected:
    // 颜色转换工具函数
    // static std::optional<std::vector<double>> convertHexToColorArray(const std::string& hexStr);
    // static std::string convertColorArrayToHex(const std::vector<double>& colorArray);
};

#endif