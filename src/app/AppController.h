#pragma once

#include <vector>
#include <atomic>
#include <rtaudio/RtAudio.h>

#include "GraphicsContext.h"
#include "audio/AudioSession.h"
#include "NoteConverter.h"
#include "AnimatedUI.h"
#include "ConfigStore.h"

class AppController
{
public:
    explicit AppController(const std::vector<RtAudio::Api> &apis);
    int run(std::atomic<bool> &quitFlag);

private:
    GraphicsContext gfx_;
    AudioSession audio_;
    ConfigStore configStore_{};
    NoteConverter noteConverter_{};
    AnimatedUI ui_{};
    std::vector<RtAudio::Api> apis_;
};
