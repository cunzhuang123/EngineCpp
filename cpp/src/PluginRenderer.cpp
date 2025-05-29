#include "PluginRenderer.h"
#include "Materials.h"
#include "ExpressTool.h"
#include "../Engine.h"

PluginRenderer::PluginRenderer(Engine& engine, const std::string& name)
    : engine(engine), name(name), isVisible(true), buffer(0), isGenerateEffect(false), hasUniformTime(false)
{

    // 创建OpenGL buffer
    glGenBuffers(1, &buffer);
}

PluginRenderer::~PluginRenderer()
{
    destroy();
}

void PluginRenderer::updateVerticeBuffer(Engine& engine)
{
    std::shared_ptr<Material> material = std::get<std::shared_ptr<Material>>(materialPass->uniforms["u_texture"].value);
    // 检查并调整attributeBuffer
    if (materialPass->renderTargetInfo.width != material->renderTargetInfo.width ||
    materialPass->renderTargetInfo.height != material->renderTargetInfo.height)
    {
        float x = (float)material->renderTargetInfo.width/(float)materialPass->renderTargetInfo.width;
        float y = (float)material->renderTargetInfo.height/(float)materialPass->renderTargetInfo.height;

        float ndcVertices[] = {
            // x,  y,  z, u, v
            -x,  y, 0.0f, 0.0f, 1.0f,
            x,  y, 0.0f, 1.0f, 1.0f,
            -x, -y, 0.0f, 0.0f, 0.0f,
            x, -y, 0.0f, 1.0f, 0.0f,
        };

        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ndcVertices), ndcVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        materialPass->attributeBuffer = buffer;
    }
    else
    {
        materialPass->attributeBuffer = engine.getNdcBuffer();
    }
}

void PluginRenderer::updatePlugin(const nlohmann::json& sequence, std::vector<nlohmann::json> plugins, const nlohmann::json& materialData, std::map<std::string, std::shared_ptr<RendererResource>> rendererResourceMap, GLuint screenBuffer, GLuint ndcBuffer)
{
    auto sequenceRenderTarget = engine.getSequenceRenderTargetInfo();
    // 创建 materialPass (基于Blit材料)
    materialPass = std::make_shared<Material>(Blit);
    materialPass->passName += "_" + name;
    materialPass->attributeBuffer = engine.getNdcBuffer();
    materialPass->renderTargetInfo = sequenceRenderTarget;

    std::string seqId = sequence["id"];
    auto material = Material::deserialize(materialData, rendererResourceMap, screenBuffer, ndcBuffer, seqId, sequenceRenderTarget);
    materialPass->uniforms["u_texture"] = { UniformType::MaterialPtr, material};
    for (int j = 0; j < plugins.size(); j++)
    {
        auto& plugin = plugins[j];
        const auto& expressValue = ExpressTool::collectMaterialExpressValue(nullptr, name, materialPass, plugin, j, sequenceRenderTarget);
        ExpressTool::caculateMaterialExpress(name, materialPass, expressValue, j);
    }

    // // 检查并调整attributeBuffer
    // if (materialPass->renderTargetInfo.width != material->renderTargetInfo.width ||
    // materialPass->renderTargetInfo.height != material->renderTargetInfo.height)
    // {
    //     float x = (float)material->renderTargetInfo.width/(float)materialPass->renderTargetInfo.width;
    //     float y = (float)material->renderTargetInfo.height/(float)materialPass->renderTargetInfo.height;

    //     float ndcVertices[] = {
    //         // x,  y,  z, u, v
    //         -x,  y, 0.0f, 0.0f, 1.0f,
    //         x,  y, 0.0f, 1.0f, 1.0f,
    //         -x, -y, 0.0f, 0.0f, 0.0f,
    //         x, -y, 0.0f, 1.0f, 0.0f,
    //     };

    //     glBindBuffer(GL_ARRAY_BUFFER, buffer);
    //     glBufferData(GL_ARRAY_BUFFER, sizeof(ndcVertices), ndcVertices, GL_STATIC_DRAW);
    //     glBindBuffer(GL_ARRAY_BUFFER, 0);

    //     materialPass->attributeBuffer = buffer;
    // }
    // else
    // {
    //     materialPass->attributeBuffer = engine.getNdcbuffer();
    // }
    updateVerticeBuffer(engine);

    if (isUse(materialPass, sequenceRenderTarget))
    {
        isGenerateEffect = false;
        std::shared_ptr<float[]> clearColor(new float[4]{0.0f, 0.0f, 0.0f, 0.0f});
        materialPass->clearColor = clearColor;
        materialPass->clear = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    }
    else 
    {
        isGenerateEffect = true;
    }
    if (hasUniform(materialPass, "time"))
    {
        hasUniformTime = true;
    }
    else 
    {
        hasUniformTime = false;
    }
}

bool PluginRenderer::isUse(std::shared_ptr<Material> pass, RenderTargetInfo renderTargetInfo)
{
    for (auto &kv : pass->uniforms) {
        UniformValue uniform = kv.second;
        // 如果该 uniform 中关联了子 Pass，则递归查找
        if (uniform.type == UniformType::RenderTarget) {
            RenderTargetInfo info = std::get<RenderTargetInfo>(uniform.value);
            if (info.name == renderTargetInfo.name)
            {
                return true;
            }
        }
    }
    for (auto &kv : pass->uniforms) {
        UniformValue uniform = kv.second;
        // 如果该 uniform 中关联了子 Pass，则递归查找
        if (uniform.type == UniformType::MaterialPtr) {
            std::shared_ptr<Material> tempPass = std::get<std::shared_ptr<Material>>(uniform.value);
            bool tempResult = isUse(tempPass, renderTargetInfo);
            if (tempResult)
            {
                return true;
            }
        }
    }
    return false;
}

bool PluginRenderer::hasUniform(std::shared_ptr<Material> pass, std::string targetUniformName)
{
    for (auto &kv : pass->uniforms) {
        if (kv.first == targetUniformName) {
            return true;
        }
    }
    for (auto &kv : pass->uniforms) {
        UniformValue uniform = kv.second;
        // 如果该 uniform 中关联了子 Pass，则递归查找
        if (uniform.type == UniformType::MaterialPtr) {
            std::shared_ptr<Material> tempPass = std::get<std::shared_ptr<Material>>(uniform.value);
            bool tempResult = hasUniform(tempPass, targetUniformName);
            if (tempResult)
            {
                return true;
            }
        }
    }
    return false;
}

void PluginRenderer::updateTime(float time)
{
    updateUniform(materialPass, "time", { UniformType::Float, time });
}

void PluginRenderer::updateUniform(std::shared_ptr<Material> pass, std::string targetUniformName, UniformValue value)
{
    for (auto &kv : pass->uniforms) {
        if (kv.first == targetUniformName) {
            pass->uniforms[targetUniformName] = value;
        }
    }
    for (auto &kv : pass->uniforms) {
        UniformValue uniform = kv.second;
        // 如果该 uniform 中关联了子 Pass，则递归查找
        if (uniform.type == UniformType::MaterialPtr) {
            std::shared_ptr<Material> tempPass = std::get<std::shared_ptr<Material>>(uniform.value);
            updateUniform(tempPass, targetUniformName, value);
        }
    }            
}

void PluginRenderer::destroy()
{
    if (buffer != 0)
    {
        glDeleteBuffers(1, &buffer);
        buffer = 0;
    }
}