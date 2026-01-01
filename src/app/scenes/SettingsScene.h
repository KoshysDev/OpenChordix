#pragma once

#include <string>
#include <utility>

#include "Scene.h"
#include "AnimatedUI.h"
#include "AudioSession.h"
#include "settings/DisplaySettingsController.h"
#include "settings/GraphicsConfig.h"
#include "ui/ModalDialog.h"
#include "ui/DeviceSelector.h"

class SettingsScene : public Scene
{
public:
    SettingsScene(AudioSession &audioSession, AnimatedUI &ui);

    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    struct AudioSettings
    {
        float masterVolume = 0.8f;
        float musicVolume = 0.6f;
        float sfxVolume = 0.9f;
        bool muteAll = false;
        bool enableInputMonitor = true;
    };

    struct GraphicsSettings
    {
        GraphicsConfig config{};
    };

    struct DeviceSettings
    {
        bool hotplugNotifications = true;
    };

    struct GameplaySettings
    {
        bool metronomeEnabled = false;
        float metronomeVolume = 0.5f;
        bool visualMetronome = true;
        bool backgroundTips = true;
        int noteSpeed = 5;
        int hitWindow = 2;
    };

    void drawHeader();
    void drawAudioTab();
    void drawGraphicsTab();
    void drawDisplayTab(GraphicsContext &gfx);
    void drawDeviceTab();
    void drawGameplayTab();
    void drawFooter(GraphicsContext &gfx);
    void applySettings(GraphicsContext &gfx);
    void applyAudio();

    AudioSession &audioSession_;
    AnimatedUI &ui_;
    GraphicsContext *lastGfx_ = nullptr;
    AudioSettings audioSettings_;
    GraphicsSettings graphics_;
    DisplaySettingsController displayController_;
    DeviceSettings devices_;
    GameplaySettings gameplay_;
    ModalDialog unsavedModal_;
    bool dirty_ = false;
    bool finished_ = false;
};
