#pragma once

#include "Scene.h"
#include "AnimatedUI.h"
#include "ui/ModalDialog.h"

class MainMenuScene : public Scene
{
public:
    enum class Action
    {
        None,
        OpenTuner,
        OpenSettings
    };

    explicit MainMenuScene(AnimatedUI &ui);
    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return false; }
    Action consumeAction()
    {
        Action a = pendingAction_;
        pendingAction_ = Action::None;
        return a;
    }

private:
    AnimatedUI &ui_;
    Action pendingAction_ = Action::None;
    ModalDialog featureModal_;
};
