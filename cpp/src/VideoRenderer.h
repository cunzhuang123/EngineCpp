
// VideoRenderer.h

#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include "Camera.h"
#include "Materials.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "RendererResource.h"
#include "TransitionRenderer.h"


class VideoRenderer {
public:
    VideoRenderer(std::shared_ptr<Camera> camera, std::shared_ptr<Camera> screenCamera, GLuint screenBuffer, const std::string& name);

    bool initialize(int rotate, RenderTargetInfo renderTargetInfo);
    void updateMaterialUniforms();
    void updateVerticeBuffer();
    glm::mat4 getModelMatrix();

    void destroy();
    void setPosition(glm::vec3& position);
    void setRotation(glm::vec3& rotation);
    void setScale(glm::vec3& scale);
    void setAnchor(glm::vec3& anchor);
    void setColor(glm::vec4& color);
    void setColorAlpha(float alpha);
    void setName(const std::string& name) {this->name = name;};
    std::string getName() {return name;};


    std::shared_ptr<Material> getMaterialPass() {
        return materialPass;
    }
    void setMaterialTexturePass(std::shared_ptr<Material> materialPass);
    void setRenderTarget(RenderTargetInfo renderTargetInfo);

    void setRendererResource(std::shared_ptr<RendererResource> rendererResource);
    std::shared_ptr<RendererResource> getRendererResource() {return rendererResource;};

    void updateRendererResource(std::shared_ptr<RendererResource> rendererResource, int rotate);

    int getWidth() const;
    int getHeight() const;
    
    glm::vec3 position;
    glm::vec3 rotation;

private:
    std::shared_ptr<RendererResource> rendererResource;
    std::shared_ptr<Material> materialPass; // 使用智能指针
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Camera> screenCamera;
    GLuint screenBuffer;
    std::string name;
    GLuint buffer;

    glm::vec3 scale;
    glm::vec3 anchor;
    glm::vec4 color;
};

#endif // VIDEORENDERER_H