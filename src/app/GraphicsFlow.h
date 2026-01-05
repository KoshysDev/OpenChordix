#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <rtaudio/RtAudio.h>

#include "AnimatedUI.h"
#include "audio/AudioSession.h"
#include "ConfigStore.h"
#include "GraphicsContext.h"
#include "NoteConverter.h"
#include "Scene.h"
#include "devtools/DevConsole.h"

class GraphicsFlow
{
public:
    GraphicsFlow(GraphicsContext &gfx,
                 AudioSession &audio,
                 ConfigStore &configStore,
                 NoteConverter &noteConverter,
                 AnimatedUI &ui,
                 const std::vector<RtAudio::Api> &apis,
                 bool enableDevTools);

    int run(std::atomic<bool> &quitFlag);

private:
    enum class SceneId
    {
        Intro,
        AudioSetup,
        MainMenu,
        TrackSelect,
        Tuner,
        Settings,
        Test
    };

    void configureImGuiStyle();
    std::unique_ptr<Scene> makeScene(SceneId id);

    GraphicsContext &gfx_;
    AudioSession &audio_;
    ConfigStore &configStore_;
    NoteConverter &noteConverter_;
    AnimatedUI &ui_;
    std::vector<RtAudio::Api> apis_;
    openchordix::devtools::DevConsole devConsole_;
};
