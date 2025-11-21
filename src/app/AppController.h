#pragma once

#include <vector>
#include <atomic>
#include <rtaudio/RtAudio.h>

#include "GraphicsContext.h"
#include "AudioSession.h"
#include "NoteConverter.h"
#include "AnimatedUI.h"

class AppController
{
public:
    explicit AppController(const std::vector<RtAudio::Api> &apis);
    int run(std::atomic<bool> &quitFlag);

private:
    void configureImGuiStyle();
    int runConsoleFlow(std::atomic<bool> &quitFlag);
    int runGraphicsFlow(std::atomic<bool> &quitFlag);

    GraphicsContext gfx_;
    AudioSession audio_;
    NoteConverter noteConverter_{};
    AnimatedUI ui_{};
    std::vector<RtAudio::Api> apis_;
};
