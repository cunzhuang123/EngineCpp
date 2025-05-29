

#include "TrackUtils.h"
#include <algorithm> // 用于 std::min
#include <iostream>  // 用于调试输出（可选）


double TrackUtils::getSequenceTime(double globalTime, const nlohmann::json& sequence) {
    const auto& timer = sequence["timer"];
    double sequenceTime = globalTime - timer["offset"].get<double>();
    return sequenceTime;
}
double TrackUtils::getNomalSequenceTime(double globalTime, const nlohmann::json& sequence) {
    const auto& timer = sequence["timer"];
    double sequenceTime = globalTime - timer["offset"].get<double>();
    double duration = timer["duration"].get<double>();
    double originalDuration = timer["originalDuration"].get<double>();
    double rate = timer["rate"].get<double>();

    // 计算序列的裁剪后持续时间
    double trimmedDuration = duration * (originalDuration / rate);

    return sequenceTime/(trimmedDuration - 33.3);
}

// 判断当前全局时间点是否在序列的可见范围内
bool TrackUtils::isVisibleAtTime(double globalTime, const nlohmann::json& sequence) {
    const auto& timer = sequence["timer"];

    double sequenceTime = globalTime - timer["offset"].get<double>();
    double duration = timer["duration"].get<double>();
    double originalDuration = timer["originalDuration"].get<double>();
    double rate = timer["rate"].get<double>();

    // 计算序列的裁剪后持续时间
    double trimmedDuration = duration * (originalDuration / rate);

    // return (sequenceTime >= 0) && (sequenceTime <= (trimmedDuration  - 33.3));
    return (sequenceTime >= 0) && (sequenceTime <= trimmedDuration);
}

// 根据全局时间点获取原始资源的时间点
double TrackUtils::getOriginalTime(double globalTime, const nlohmann::json& sequence) {
    const auto& timer = sequence["timer"];

    double sequenceTime = globalTime - timer["offset"].get<double>();
    double startPercent = timer["start"].get<double>();
    double originalDuration = timer["originalDuration"].get<double>();
    double rate = timer["rate"].get<double>();

    // 计算原始资源的起始时间和原始时间点
    double originalStartTime = startPercent * originalDuration;
    double originalTime = sequenceTime * rate + originalStartTime;

    if (originalTime < originalStartTime)
    {
        return originalStartTime;
    }
    else if (originalTime > originalDuration)
    {
        return originalDuration;
    }
    else 
    {
        return originalTime;
    }
}


glm::vec3 TrackUtils::getSequenceScale(float sourceHeight, float renderTargetHeight, 
                                   float sourceWidth, float renderTargetWidth, 
                                   const std::array<float, 2>& sequenceScale) {
    float scaleWidth = renderTargetWidth / sourceWidth;
    float scaleHeight = renderTargetHeight / sourceHeight;
    float scale = std::min(scaleWidth, scaleHeight);

    auto [sequenceScaleX, sequenceScaleY] = sequenceScale;

    glm::vec3 result;
    result.x = scale * sequenceScaleX;
    result.y = scale * sequenceScaleY;
    result.z = 1.0f; // 固定为 1.0

    return result;
}

double TrackUtils::getTransitionTime(double globalTime, const nlohmann::json& transition, const nlohmann::json& sequence) {
    const auto& timer = sequence["timer"];

    double sequenceEndTime = timer["offset"].get<double>() + (timer["duration"].get<double>()*(timer["originalDuration"].get<double>()/timer["rate"].get<double>()));
    double transitionTime = globalTime - (sequenceEndTime - transition["duration"].get<double>()/2);
    return transitionTime;
}

