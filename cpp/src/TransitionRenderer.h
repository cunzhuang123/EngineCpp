// TransitionRenderer.h

#ifndef TRANSITION_RENDERER_H
#define TRANSITION_RENDERER_H

#include <string>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "Materials.h"


class Engine;
class VideoRenderer;

class TransitionRenderer {
public:
    TransitionRenderer(std::shared_ptr<VideoRenderer> firstRenderer, std::shared_ptr<VideoRenderer> secondRenderer, std::string id, Engine& engine);
    virtual ~TransitionRenderer();

    void updateTime(double time);
    void updateRenderTargetInfo(RenderTargetInfo renderTargetInfo);
    std::shared_ptr<VideoRenderer> firstRenderer;
    std::shared_ptr<VideoRenderer> secondRenderer;
    std::shared_ptr<Material> getMaterialPass() {
        return materialPass;
    }

    virtual void setMaterialPass(std::shared_ptr<Material> materialPass)
    {
        this->materialPass = materialPass;
    }


private:
    std::string id;
    std::shared_ptr<Material> materialPass;
};

#endif // TRANSITION_RENDERER_H