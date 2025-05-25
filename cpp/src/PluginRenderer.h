#ifndef PLUGIN_RENDERER_H
#define PLUGIN_RENDERER_H


#include <memory>
#include <string>
#include "Materials.h"
#include "VideoRenderer.h"

class Engine;

class PluginRenderer {
public:
    PluginRenderer(Engine& engine, const std::string& name);
    ~PluginRenderer();

    void updateVerticeBuffer(Engine& engine);
    void updatePlugin(const nlohmann::json& sequence, std::vector<nlohmann::json> plugins, const nlohmann::json& materialData, std::map<std::string, std::shared_ptr<RendererResource>> rendererResourceMap, GLuint screenBuffer, GLuint ndcBuffer);
    void setVisible(bool isVisible) { isVisible = isVisible; };
    void destroy();

    bool getVisible() const { return isVisible; }
    std::shared_ptr<Material> getMaterialPass() {
        return materialPass;
    }

    void setName(const std::string& name) {this->name = name;};
    std::string getName() {return name;};

    bool isUse(std::shared_ptr<Material> pass, RenderTargetInfo renderTargetInfo);
    bool hasUniform(std::shared_ptr<Material> pass, std::string targetUniformName);

    void updateTime(float time);

    void updateUniform(std::shared_ptr<Material> pass, std::string targetUniformName, UniformValue value);

    bool isGenerateEffect;
    bool hasUniformTime;
private:
    Engine& engine;
    std::string name;
    bool isVisible;
    std::shared_ptr<Material> materialPass;

    unsigned int buffer; // OpenGL buffer ID
};


#endif // PLUGIN_RENDERER_H
