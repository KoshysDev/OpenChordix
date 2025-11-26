#pragma once

#include "Scene.h"
#include "AudioSession.h"
#include "NoteConverter.h"
#include "AnimatedUI.h"

class AudioSetupScene : public Scene
{
public:
    AudioSetupScene(AudioSession &audio, NoteConverter &noteConverter, AnimatedUI &ui);
    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    void drawInputDeviceList();
    void drawOutputDeviceList();

    AudioSession &audio_;
    NoteConverter &noteConverter_;
    AnimatedUI &ui_;
    bool finished_ = false;
};
