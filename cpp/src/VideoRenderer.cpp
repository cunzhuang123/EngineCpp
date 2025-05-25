

// VideoRenderer.cpp

#include "VideoRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "VideoResource.h"
#include "ScopedProfiler.h"
#include "TextResource.h" 

VideoRenderer::VideoRenderer(std::shared_ptr<Camera> camera, std::shared_ptr<Camera> screenCamera, GLuint screenBuffer, const std::string& name)
    : camera(camera), screenCamera(screenCamera), screenBuffer(screenBuffer), name(name) {

    position = glm::vec3(0.0f);
    rotation = glm::vec3(0.0f);
    scale = glm::vec3(0.5f);
    anchor = glm::vec3(0.0f);

    buffer = 0;
}

void VideoRenderer::setRendererResource(std::shared_ptr<RendererResource> rendererResource)
{
    this->rendererResource = rendererResource;
}
bool VideoRenderer::initialize(int rotate, RenderTargetInfo renderTargetInfo) {
    if (rendererResource->initialize(rotate)) {
        glGenBuffers(1, &buffer);
        // glBindBuffer(GL_ARRAY_BUFFER, buffer);

        // const std::vector<float>& vertices = rendererResource->getVertices();  // 使用常量引用，避免拷贝
        // glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // 同样使用智能指针创建 materialPass
        materialPass = std::make_shared<Material>(Stand);
        materialPass->passName += ("_" + name);
        materialPass->attributeBuffer = buffer;
        materialPass->uniforms["u_texture"].type = UniformType::Texture2D;
        materialPass->uniforms["u_texture"].value = rendererResource->getTexture();//.get(); // 存储 Material* 指针
        materialPass->renderTargetInfo = renderTargetInfo; // 渲染到屏幕

        updateMaterialUniforms();

        // materialPass = sourcePass;
        return true;
    }
    return false;
}

void VideoRenderer::updateRendererResource(std::shared_ptr<RendererResource> newRendererResource, int rotate)
{
    if (newRendererResource->initialize(rotate))
    {
        Material::updateTextrue(materialPass, rendererResource->getTexture(), newRendererResource->getTexture());

        std::shared_ptr<TextResource> textResource = std::dynamic_pointer_cast<TextResource>(rendererResource);
        if (textResource)
        {
            auto rtKey = RenderTargetPool::makeRTKey(textResource->getWidth(), textResource->getHeight(), true);
            RenderTargetPool::instance().cachedRenderTarget(rtKey, textResource->getRenderTarget());
        }
        setRendererResource(newRendererResource);
        updateMaterialUniforms();
        updateVerticeBuffer();
    }
}

void VideoRenderer::setMaterialTexturePass(std::shared_ptr<Material> newMaterialPass) {
    materialPass->uniforms["u_texture"].type = UniformType::MaterialPtr;
    materialPass->uniforms["u_texture"].value = newMaterialPass;
}

void VideoRenderer::setRenderTarget(RenderTargetInfo renderTargetInfo) {
    materialPass->renderTargetInfo = renderTargetInfo;
}


void VideoRenderer::setPosition(glm::vec3& position) { this->position = position; };
void VideoRenderer::setRotation(glm::vec3& rotation) { this->rotation = rotation; };
void VideoRenderer::setScale(glm::vec3& scale) { this->scale = scale; };
void VideoRenderer::setAnchor(glm::vec3& anchor) { this->anchor = anchor; };
void VideoRenderer::setColor(glm::vec4& color) { this->color = color; };
void VideoRenderer::setColorAlpha(float alpha) { this->color.a = alpha; };

void VideoRenderer::updateMaterialUniforms() {

    // glm::mat4 vec4f = std::get<glm::mat4>(materialPass->uniforms["u_modelMatrix"].value);
    // std::cerr << "old:" << glm::to_string(vec4f) << std::endl;

    materialPass->uniforms["u_modelMatrix"].value = getModelMatrix();

    // vec4f = std::get<glm::mat4>(materialPass->uniforms["u_modelMatrix"].value);
    // std::cerr << "new:" << glm::to_string(vec4f) << std::endl;

    glm::mat4 screenViewMatrix = camera->getViewMatrix();
    materialPass->uniforms["u_viewMatrix"].value = screenViewMatrix;
    glm::mat4 screenProjectionMatrix = camera->getProjectionMatrix();
    materialPass->uniforms["u_projectionMatrix"].value = screenProjectionMatrix;
    materialPass->uniforms["u_color"].value = color;
}

void VideoRenderer::updateVerticeBuffer()
{
    // ScopedProfiler profiler("VideoRenderer::updateVerticeBuffer " + getName());

    RenderTargetInfo renderTargetInfo;
    if (materialPass->uniforms["u_texture"].type == UniformType::MaterialPtr)
    {
        std::shared_ptr<Material> dependentPass = std::get<std::shared_ptr<Material>>(materialPass->uniforms["u_texture"].value);
        renderTargetInfo = dependentPass->renderTargetInfo;
    }
    else if (materialPass->uniforms["u_texture"].type == UniformType::RenderTarget)
    {
        renderTargetInfo = std::get<RenderTargetInfo>(materialPass->uniforms["u_texture"].value);
    }
    else{
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        const std::vector<float>& vertices = rendererResource->getVertices();  // 使用常量引用，避免拷贝
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return;
    }

    if (rendererResource->getWidth() != renderTargetInfo.width || rendererResource->getHeight() != renderTargetInfo.height)
    {
        // 取出渲染目标的一半宽高
        float w = renderTargetInfo.width / 2.0f;
        float h = renderTargetInfo.height / 2.0f;

        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        std::vector<float> vertices = rendererResource->getVertices();
        vertices[0]  = (vertices[0]  / std::fabs(vertices[0]))  * w;
        vertices[5]  = (vertices[5]  / std::fabs(vertices[5]))  * w;
        vertices[10] = (vertices[10] / std::fabs(vertices[10])) * w;
        vertices[15] = (vertices[15] / std::fabs(vertices[15])) * w;

        // 对 y 坐标进行缩放处理：
        vertices[1]  = (vertices[1]  / std::fabs(vertices[1]))  * h;
        vertices[6]  = (vertices[6]  / std::fabs(vertices[6]))  * h;
        vertices[11] = (vertices[11] / std::fabs(vertices[11])) * h;
        vertices[16] = (vertices[16] / std::fabs(vertices[16])) * h;

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else{
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        const std::vector<float>& vertices = rendererResource->getVertices();  // 使用常量引用，避免拷贝
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

glm::mat4 VideoRenderer::getModelMatrix() {
    glm::mat4 modelMatrix(1.0f);

    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, rotation.x, glm::vec3(1, 0, 0));
    modelMatrix = glm::rotate(modelMatrix, rotation.y, glm::vec3(0, 1, 0));

    std::shared_ptr<VideoResource> videoResource = std::dynamic_pointer_cast<VideoResource>(rendererResource);
    if (videoResource)
    {
        float rotationRadians = videoResource->getRotation() * static_cast<float>(M_PI / 180.0f);
        modelMatrix = glm::rotate(modelMatrix, -rotation.z + rotationRadians, glm::vec3(0, 0, 1));
    }
    else 
    {
        modelMatrix = glm::rotate(modelMatrix, -rotation.z, glm::vec3(0, 0, 1));
    }

    modelMatrix = glm::scale(modelMatrix, scale);
    modelMatrix = glm::translate(modelMatrix, -anchor);

    return modelMatrix;
}

int VideoRenderer::getWidth() const {
    if (materialPass->uniforms["u_texture"].type == UniformType::MaterialPtr)
    {
        std::shared_ptr<Material> dependentPass = std::get<std::shared_ptr<Material>>(materialPass->uniforms["u_texture"].value);
        return dependentPass->renderTargetInfo.width;
    }
    else if (materialPass->uniforms["u_texture"].type == UniformType::RenderTarget)
    {
        return std::get<RenderTargetInfo>(materialPass->uniforms["u_texture"].value).width;
    }
    else{
        return rendererResource->getWidth();
    }
}

int VideoRenderer::getHeight() const {
    if (materialPass->uniforms["u_texture"].type == UniformType::MaterialPtr)
    {
        std::shared_ptr<Material> dependentPass = std::get<std::shared_ptr<Material>>(materialPass->uniforms["u_texture"].value);
        return dependentPass->renderTargetInfo.height;
    }
    else if (materialPass->uniforms["u_texture"].type == UniformType::RenderTarget)
    {
        return std::get<RenderTargetInfo>(materialPass->uniforms["u_texture"].value).height;
    }
    else{
        return rendererResource->getHeight();
    }
}



void VideoRenderer::destroy() {
    if (buffer != 0) {
        glDeleteBuffers(1, &buffer);
        buffer = 0;
    }
}