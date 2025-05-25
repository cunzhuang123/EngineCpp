#ifndef TRACK_UTILS_H
#define TRACK_UTILS_H

#include "nlohmann/json.hpp"
#include <glm/glm.hpp>

class TrackUtils {
public:
    TrackUtils() {}
    ~TrackUtils() {}

    double getSequenceTime(double globalTime, const nlohmann::json& sequence);

    double getNomalSequenceTime(double globalTime, const nlohmann::json& sequence);
    
    // 判断序列在指定时间是否可见
    bool isVisibleAtTime(double realTime, const nlohmann::json& sequence);

    // 计算真实时间
    double getOriginalTime(double globalTime, const nlohmann::json& sequence);

    // 计算序列的缩放比例
    glm::vec3 getSequenceScale(float sourceHeight, float renderTargetHeight, 
                           float sourceWidth, float renderTargetWidth, 
                           const std::array<float, 2>& sequenceScale);

    double getTransitionTime(double globalTime, const nlohmann::json& transition, const nlohmann::json& sequence);
};

#endif // TRACK_UTILS_H