#pragma once

#include <atomic>

#include "GraphicsContext.h"

class Scene
{
public:
    virtual ~Scene() = default;
    virtual void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) = 0;
    virtual bool finished() const = 0;
};
