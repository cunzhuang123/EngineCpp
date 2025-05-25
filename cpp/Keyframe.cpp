// Keyframe.cpp
#include "Keyframe.h"
#include <cmath>
#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "Engine.h"
#include "src/VideoRenderer.h"
#include "src/TextResource.h"  // 假设存在文本资源类
#include "src/ExpressTool.h"
#include "src/ScopedProfiler.h"


nlohmann::json Keyframe::getKeyframeValue(const nlohmann::json& keyframeArray, double globalTime, Engine& engine) {
    if (!keyframeArray.is_array() || keyframeArray.empty())
        return {};

    const nlohmann::json* preKeyframe = nullptr;

    for (const auto& keyframe : keyframeArray) {
        if (!keyframe.contains("offset") || !keyframe.contains("value") || !keyframe.contains("type"))
            continue; // 忽略无效关键帧

        double offset = keyframe["offset"].get<double>();

        if (globalTime < offset) {
            if (!preKeyframe) {
                // 如果这是第一个关键帧且当前时间小于第一个关键帧的offset，直接返回当前关键帧值
                return keyframe["value"];
            } else {
                // 获取当前关键帧和前一个关键帧的值
                auto curValJson = keyframe["value"];
                auto preValJson = (*preKeyframe)["value"];

                // 尝试转换成颜色数组
                std::optional<std::array<double, 4>> curColorOpt, preColorOpt;

                if (curValJson.is_string())
                    curColorOpt = CoreUtils::convertHexToColorArray(curValJson.get<std::string>());
                if (preValJson.is_string())
                    preColorOpt = CoreUtils::convertHexToColorArray(preValJson.get<std::string>());

                double factor = (globalTime - (*preKeyframe)["offset"].get<double>()) /
                                (offset - (*preKeyframe)["offset"].get<double>());

                if (curColorOpt && preColorOpt && curColorOpt->size() == preColorOpt->size()) {
                    // 颜色插值
                    std::vector<double> interpolated(curColorOpt->size());
                    for (size_t i = 0; i < interpolated.size(); i++) {
                        interpolated[i] = factor * ((*curColorOpt)[i] - (*preColorOpt)[i]) + (*preColorOpt)[i];
                    }
                    return CoreUtils::convertColorArrayToHex(interpolated);
                } else if (curValJson.is_number() && preValJson.is_number()) {
                    // 数值插值
                    double curNum = curValJson.get<double>();
                    double preNum = preValJson.get<double>();
                    double interpolatedNum = factor * (curNum - preNum) + preNum;
                    return interpolatedNum;
                } else {
                    // 类型不匹配或无法插值时直接返回前一个值
                    return preValJson;
                }
            }
        }
        preKeyframe = &keyframe;
    }

    // 如果globalTime超过所有关键帧，返回最后一个关键帧的值
    return (*preKeyframe)["value"];
}

void Keyframe::updateRendererAdjust(VideoRenderer& renderer, double globalTime, const nlohmann::json& sequence, Engine& engine) {
    // ScopedProfiler profiler("Keyframe::updateRendererAdjust " + renderer.getName());

    if (!sequence.contains("keyframe") || !sequence["keyframe"].is_object()) return;
    auto kf = sequence["keyframe"];
    
    if (kf.contains("adjust.transform.x") && kf["adjust.transform.x"].is_array()) {
        double x = getKeyframeValue(kf["adjust.transform.x"], globalTime, engine).get<double>();
        renderer.position.x = static_cast<float>(x * engine.getRenderTargetWidth());
    }
    
    if (kf.contains("adjust.transform.y") && kf["adjust.transform.y"].is_array()) {
        double y = getKeyframeValue(kf["adjust.transform.y"], globalTime, engine).get<double>();
        renderer.position.y = static_cast<float>(-y * engine.getRenderTargetHeight());
    }
    
    if (kf.contains("adjust.rotate") && kf["adjust.rotate"].is_array()) {
        double rotate = getKeyframeValue(kf["adjust.rotate"], globalTime, engine).get<double>();
        renderer.rotation = glm::vec3(0, 0, static_cast<float>(rotate * M_PI / 180.0));
    }
    
    // 处理缩放（排除文本资源）
    if (!std::dynamic_pointer_cast<TextResource>(renderer.getRendererResource())) {
        glm::vec3 scale(1.0f);
        bool hasScale = false;
        
        if (kf.contains("adjust.scale.x") && kf["adjust.scale.x"].is_array()) {
            scale.x = static_cast<float>(getKeyframeValue(kf["adjust.scale.x"], globalTime, engine).get<double>());
            hasScale = true;
        }
        
        if (kf.contains("adjust.scale.y") && kf["adjust.scale.y"].is_array()) {
            scale.y = static_cast<float>(getKeyframeValue(kf["adjust.scale.y"], globalTime, engine).get<double>());
            hasScale = true;
        }
        
        if (hasScale) {
            renderer.setScale(scale);
        }
    }
    
    // 处理透明度
    if (kf.contains("adjust.opacity") && kf["adjust.opacity"].is_array()) {
        double opacity = getKeyframeValue(kf["adjust.opacity"], globalTime, engine).get<double>();
        renderer.setColorAlpha(static_cast<float>(opacity));
    }
    
    renderer.updateMaterialUniforms();
}

void Keyframe::updateTextRenderer(VideoRenderer& renderer, double globalTime, const nlohmann::json& sequence, Engine& engine) {    
    // ScopedProfiler profiler("Keyframe::updateTextRenderer " + renderer.getName());
    auto kf = sequence.value("keyframe", nlohmann::json::object());
    glm::vec2 scale(1.0f);
    bool hasValue = false;
    
    if (kf.contains("adjust.scale.x") && kf["adjust.scale.x"].is_array()) {
        scale.x = static_cast<float>(getKeyframeValue(kf["adjust.scale.x"], globalTime, engine).get<double>());
        hasValue = true;
    }
    
    if (kf.contains("adjust.scale.y") && kf["adjust.scale.y"].is_array()) {
        scale.y = static_cast<float>(getKeyframeValue(kf["adjust.scale.y"], globalTime, engine).get<double>());
        hasValue = true;
    }
    
    // 处理文本资源属性
    const nlohmann::json sequenceResource = sequence["resource"];
    bool isStroke = sequenceResource.value("strokeEnabled", false);

    std::optional<std::array<double, 4>> color = CoreUtils::convertHexToColorArray(sequenceResource.value("color", "#db1116ff"));
    std::optional<std::array<double, 4>> strokeColor = CoreUtils::convertHexToColorArray(sequenceResource.value("strokeColor", "#db1116ff"));
    int strokeWidth = isStroke ? sequenceResource.value("strokeWidth", 0) : 0;
    int fontSize = static_cast<int>(sequenceResource.value("fontSize", 60)*scale.x);

    if (kf.contains("resource.fontSize") && kf["resource.fontSize"].is_array()) {
        fontSize = static_cast<int>(getKeyframeValue(kf["resource.fontSize"], globalTime, engine).get<double>()*scale.x);
        hasValue = true;
    }
    
    if (kf.contains("resource.strokeWidth") && kf["resource.strokeWidth"].is_array()) {
        strokeWidth = isStroke ? static_cast<int>(getKeyframeValue(kf["resource.strokeWidth"], globalTime, engine).get<double>()) : 0;
        hasValue = true;
    }
    
    if (kf.contains("resource.color") && kf["resource.color"].is_array()) {
        color = CoreUtils::convertHexToColorArray(getKeyframeValue(kf["resource.color"], globalTime, engine).get<std::string>());
        hasValue = true;
    }
    
    if (kf.contains("resource.strokeColor") && kf["resource.strokeColor"].is_array()) {
        strokeColor = CoreUtils::convertHexToColorArray(getKeyframeValue(kf["resource.strokeColor"], globalTime, engine).get<std::string>());
        hasValue = true;
    }
    
    if (hasValue) {
        std::string resourcePath = sequenceResource.value("absolutePath", "");
        auto resource = std::make_shared<TextResource>(resourcePath, sequenceResource.value("text", "xxx"), fontSize, color, strokeWidth, strokeColor);
        renderer.updateRendererResource(resource, 0);
    }
}

nlohmann::json Keyframe::getKeyframeControl(const nlohmann::json& plugin, double globalTime, Engine& engine)
{
    nlohmann::json keyframeControl = nlohmann::json::object();
    
    if (!plugin.contains("control") || !plugin["control"].is_object()) return keyframeControl;
    
    for (auto& [key, value] : plugin["control"].items()) {
        if (value.is_array()) {
            keyframeControl[key] = nlohmann::json::array();//value.size()
            for (size_t i = 0; i < value.size(); ++i) {
                std::string controlKey = "control." + key + "[" + std::to_string(i) + "]";
                if (plugin.contains("keyframe") && plugin["keyframe"].contains(controlKey)) {
                    keyframeControl[key][i] = getKeyframeValue(plugin["keyframe"][controlKey], globalTime, engine);
                } else {
                    keyframeControl[key][i] = value[i];
                }
            }
        } else {
            std::string controlKey = "control." + key;
            if (plugin.contains("keyframe") && plugin["keyframe"].contains(controlKey)) {
                keyframeControl[key] = getKeyframeValue(plugin["keyframe"][controlKey], globalTime, engine);
            } else {
                keyframeControl[key] = value;
            }
        }
    }
    return keyframeControl;
}

void Keyframe::updateRendererPlugin(VideoRenderer& renderer, double globalTime, const nlohmann::json& sequence, Engine& engine, const nlohmann::json& plugin, int pluginIndex) {
    // ScopedProfiler profiler("Keyframe::updateRendererPlugin " + renderer.getName());
    nlohmann::json keyframeControl = getKeyframeControl(plugin, globalTime, engine);
    try {
        nlohmann::json tempPlugin = nlohmann::json::object();
        tempPlugin["control"] = keyframeControl;
        const auto& expressValue = ExpressTool::collectMaterialExpressValue(renderer.getRendererResource(), renderer.getName(), renderer.getMaterialPass(), tempPlugin, pluginIndex, engine.getSequenceRenderTargetInfo());
        ExpressTool::caculateMaterialExpress(renderer.getName(), renderer.getMaterialPass(), expressValue, pluginIndex);
    } catch (const std::exception& e) {
        std::cerr << "片段（id=" << sequence.value("id", "") << "）的插件（id=" << plugin.value("id", "") << "）关键帧属性报错:" << e.what() << std::endl;
    }
}

bool Keyframe::isPlainNoEmptyObject(const nlohmann::json& val) {
    return val.is_object() && !val.empty();
}

void Keyframe::updateRenderer(VideoRenderer& renderer, double globalTime, const nlohmann::json& sequence, Engine& engine) {
    // ScopedProfiler profiler("Keyframe::updateRenderer " + renderer.getName());
    if (sequence.contains("keyframe") && isPlainNoEmptyObject(sequence["keyframe"])) {
        updateRendererAdjust(renderer, globalTime, sequence, engine);

        std::shared_ptr<RendererResource> resource = renderer.getRendererResource();
        std::shared_ptr<TextResource> textResource = std::dynamic_pointer_cast<TextResource>(resource);
        if (textResource) {
            updateTextRenderer(renderer, globalTime, sequence, engine);
        }
    }
    if (sequence.contains("plugins") && sequence["plugins"].is_array()) {
        bool hasValue = false;
        const auto& plugins = sequence["plugins"];
        
        for (size_t i = 0; i < plugins.size(); ++i) {
            const auto& plugin = plugins[i];
            if (isPlainNoEmptyObject(plugin)) {
                updateRendererPlugin(renderer, globalTime, sequence, engine, plugin, static_cast<int>(i));
                hasValue = true;
            }
        }
        if (hasValue) {// && renderer.materialTexture.renderTa
            renderer.updateVerticeBuffer();
        }
    }
}

void Keyframe::updatePluginRenderer(PluginRenderer& pluginRenderer, double globalTime, const nlohmann::json& sequence, Engine& engine)
{
    auto sequenceRenderTarget = engine.getSequenceRenderTargetInfo();
    const auto& plugins = sequence["plugins"];
    for (size_t j = 0; j < plugins.size(); j++)
    {
        auto& plugin = plugins[j];
        nlohmann::json keyframeControl = getKeyframeControl(plugin, globalTime, engine);
        nlohmann::json tempPlugin = nlohmann::json::object();
        tempPlugin["control"] = keyframeControl;

        auto name = pluginRenderer.getName();
        const auto& expressValue = ExpressTool::collectMaterialExpressValue(nullptr, name, pluginRenderer.getMaterialPass(), tempPlugin, static_cast<int>(j), sequenceRenderTarget);
        ExpressTool::caculateMaterialExpress(name, pluginRenderer.getMaterialPass(), expressValue, static_cast<int>(j));
    }
    pluginRenderer.updateVerticeBuffer(engine);
}
