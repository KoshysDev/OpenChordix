#pragma once

#include "Scene.h"
#include "audio/AudioSession.h"
#include "NoteConverter.h"
#include "AnimatedUI.h"

#include <vector>
#include <rtaudio/RtAudio.h>

class AudioSetupScene : public Scene
{
public:
    AudioSetupScene(AudioSession &audio,
                    NoteConverter &noteConverter,
                    AnimatedUI &ui,
                    const std::vector<RtAudio::Api> &apis);
    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    void drawInputDeviceList();
    void drawOutputDeviceList();

    AudioSession &audio_;
    NoteConverter &noteConverter_;
    AnimatedUI &ui_;
    std::vector<RtAudio::Api> apiChoices_;
    bool finished_ = false;
};
