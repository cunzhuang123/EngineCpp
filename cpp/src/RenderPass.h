// RenderPass.h

#ifndef RENDERPASS_H
#define RENDERPASS_H

#include <glad/glad.h>
#include <map>
#include <string>
#include <set>
#include <vector>
#include <memory>
#include "ShaderManager.h"
#include "RenderTargetPool.h"
#include "Materials.h"
#include "VideoRenderer.h"

class RenderPass {
public:
    RenderPass(std::shared_ptr<ShaderManager> shaderManager, GLuint width, GLuint height, RenderTargetInfo defaultRenderTargetInfo, GLuint defaultFramebuffer);
    ~RenderPass();

    void render(std::vector<std::shared_ptr<Material>>& videoRendererMaterials, bool isRelease = true);

    // std::shared_ptr<RenderTargetPool> getRenderTargetPool() { return renderTargetPool; };
private:
    void renderPass(std::shared_ptr<Material> pass, std::set<std::string>& renderedSet);
    void renderSinglePass(std::shared_ptr<Material> pass);

    std::shared_ptr<ShaderManager> shaderManager;
    GLuint width;
    GLuint height;
    // std::shared_ptr<RenderTargetPool> renderTargetPool;
    RenderTargetPool& renderTargetPool;
    std::map<std::string, GLuint> programCache;
    std::set<std::string> renderedSet; // 防止重复渲染
};

#endif // RENDERPASS_H