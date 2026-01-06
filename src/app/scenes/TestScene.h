#pragma once

#include "AnimatedUI.h"
#include "Scene.h"

class TestScene : public Scene
{
public:
    explicit TestScene(AnimatedUI &ui);

    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    AnimatedUI &ui_;
    bool finished_ = false;
};
