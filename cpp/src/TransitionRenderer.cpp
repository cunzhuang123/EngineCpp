#include "TransitionRenderer.h"
#include "../Engine.h"
#include "VideoRenderer.h"


TransitionRenderer::TransitionRenderer(std::shared_ptr<VideoRenderer> firstRenderer, std::shared_ptr<VideoRenderer> secondRenderer, std::string id, Engine& engine):
firstRenderer(firstRenderer), secondRenderer(secondRenderer), id(id)
{
    materialPass = std::make_shared<Material>();
    materialPass->passName = "Transition" + id;
    materialPass->renderTargetInfo = engine.getSequenceRenderTargetInfo();
    materialPass->attributeBuffer = engine.getNdcbuffer();

    RenderTargetInfo firstRenderTarget = {id + "_firstRenderTarget",  engine.getRenderTargetWidth(),  engine.getRenderTargetHeight()};
    RenderTargetInfo secondRenderTarget = {id + "_secondRenderTarget", engine.getRenderTargetWidth(),  engine.getRenderTargetHeight()};

    materialPass->uniforms["u_firstTexture"].type = UniformType::RenderTarget;
    materialPass->uniforms["u_firstTexture"].value = firstRenderTarget;
    materialPass->uniforms["u_secondTexture"].type = UniformType::RenderTarget;
    materialPass->uniforms["u_secondTexture"].value = secondRenderTarget;
    materialPass->uniforms["u_time"].type = UniformType::Float;

}
TransitionRenderer::~TransitionRenderer()
{

}

void TransitionRenderer::updateTime(double time)
{
    materialPass->uniforms["u_time"].value = (float)time;
}

void TransitionRenderer::updateRenderTargetInfo(RenderTargetInfo renderTargetInfo)
{
    if (renderTargetInfo.name != "")
    {
        firstRenderer->setRenderTarget(renderTargetInfo);
        secondRenderer->setRenderTarget(renderTargetInfo);    
    }
    else{
        RenderTargetInfo firstRenderTarget = std::get<RenderTargetInfo>(materialPass->uniforms["u_firstTexture"].value);
        RenderTargetInfo secondTextureTarget = std::get<RenderTargetInfo>(materialPass->uniforms["u_secondTexture"].value);
        firstRenderer->setRenderTarget(firstRenderTarget);
        secondRenderer->setRenderTarget(secondTextureTarget);    
    }
}
