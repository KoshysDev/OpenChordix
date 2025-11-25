#pragma once

#include "Scene.h"
#include "AnimatedUI.h"

class MainMenuScene : public Scene
{
public:
    explicit MainMenuScene(AnimatedUI &ui) : ui_(ui) {}
    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return false; }

private:
    AnimatedUI &ui_;
};
