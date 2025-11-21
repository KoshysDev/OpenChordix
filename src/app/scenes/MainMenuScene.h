#pragma once

#include "Scene.h"

class MainMenuScene : public Scene
{
public:
    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return false; }
};
