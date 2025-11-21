#pragma once

#include "Scene.h"

class IntroScene : public Scene
{
public:
    explicit IntroScene(float durationSec = 1.5f);
    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    float duration_;
    float elapsed_ = 0.0f;
    bool finished_ = false;
};
