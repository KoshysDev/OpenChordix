#include "SettingsScene.h"

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>

#include "ui/UILayout.h"

namespace
{
    const ImVec4 kAccent = ImVec4(0.32f, 0.72f, 0.96f, 1.0f);
    const ImVec4 kMuted = ImVec4(0.70f, 0.78f, 0.90f, 1.0f);
}

SettingsScene::SettingsScene(AudioSession &audioSession, AnimatedUI &ui)
    : audioSession_(audioSession),
      ui_(ui),
      unsavedModal_(ModalDialog::confirmDiscard("unsaved_changes",
                                                "Unsaved changes",
                                                {"You have unapplied changes.", "Apply now or discard to return to the menu."},
                                                "Apply changes",
                                                "Discard",
                                                "Stay"))
{
}

void SettingsScene::drawHeader()
{
    ui_.heading("Settings", kAccent);
    ImGui::Text("Fine-tune audio, visuals, and devices. Changes apply immediately when you hit Apply.");
    ImGui::Spacing();
}

void SettingsScene::drawAudioTab()
{
    ImGui::TextDisabled("Mix");
    if (ImGui::Checkbox("Mute all", &audioSettings_.muteAll))
    {
        dirty_ = true;
    }
    ImGui::BeginDisabled(audioSettings_.muteAll);
    if (ImGui::SliderFloat("Master volume", &audioSettings_.masterVolume, 0.0f, 1.0f, "%.2f"))
    {
        dirty_ = true;
    }
    if (ImGui::SliderFloat("Music volume", &audioSettings_.musicVolume, 0.0f, 1.0f, "%.2f"))
    {
        dirty_ = true;
    }
    if (ImGui::SliderFloat("SFX volume", &audioSettings_.sfxVolume, 0.0f, 1.0f, "%.2f"))
    {
        dirty_ = true;
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::TextDisabled("Input");
    if (ImGui::Checkbox("Enable input monitoring", &audioSettings_.enableInputMonitor))
    {
        dirty_ = true;
    }
    ImGui::TextDisabled("Routing");
    ImGui::SameLine();
    ImGui::Text("%s", audioSettings_.enableInputMonitor ? "On" : "Off");
}

void SettingsScene::drawGraphicsTab()
{
    ImGui::TextDisabled("Rendering");
    if (ImGui::Checkbox("VSync", &graphics_.config.vsync))
    {
        dirty_ = true;
    }
    const char *qualityLabels[] = {"Low", "Medium", "High", "Ultra"};
    int qualityCount = static_cast<int>(sizeof(qualityLabels) / sizeof(qualityLabels[0]));
    if (ImGui::SliderInt("Quality preset", &graphics_.config.quality, 0, qualityCount - 1, qualityLabels[graphics_.config.quality]))
    {
        dirty_ = true;
    }
    if (ImGui::SliderFloat("Gamma", &graphics_.config.gamma, 1.4f, 2.6f, "%.2f"))
    {
        dirty_ = true;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Rhythm clarity");
    ImGui::TextColored(kMuted, "Tune note readability instead of cinematic effects.");
    if (ImGui::SliderInt("Note lane contrast", &graphics_.config.laneContrast, 0, 3))
    {
        dirty_ = true;
    }
    if (ImGui::SliderInt("Highway glow", &graphics_.config.highwayGlow, 0, 3))
    {
        dirty_ = true;
    }
    if (ImGui::SliderInt("Hit feedback intensity", &graphics_.config.hitFeedback, 0, 3))
    {
        dirty_ = true;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Debug overlays");
    ImGui::TextColored(kMuted, "Frame time, GPU timings, and metrics can be added here.");
}

void SettingsScene::drawDisplayTab(GraphicsContext &gfx)
{
    displayController_.refresh(gfx);
    auto &display = displayController_.state();

    if (!display.monitorNames.empty())
    {
        const char *currentMonitorLabel = display.monitorNames[display.monitorIndex].c_str();
        if (ImGui::BeginCombo("Monitor", currentMonitorLabel))
        {
            for (size_t i = 0; i < display.monitorNames.size(); ++i)
            {
                bool selected = static_cast<int>(i) == display.monitorIndex;
                if (ImGui::Selectable(display.monitorNames[i].c_str(), selected))
                {
                    display.monitorIndex = static_cast<int>(i);
                    display.resolutionIndex = 0;
                    display.refreshIndex = 0;
                    display.monitorLocked = true;
                    displayController_.invalidateOptions();
                    displayController_.markDirty();
                    dirty_ = true;
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    if (ImGui::BeginCombo("Display mode", DisplaySettingsController::modeLabel(display.modeIndex)))
    {
        for (int i = 0; i < 3; ++i)
        {
            bool selected = display.modeIndex == i;
            if (ImGui::Selectable(DisplaySettingsController::modeLabel(i), selected))
            {
                display.modeIndex = i;
                displayController_.markDirty();
                dirty_ = true;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (!display.resolutions.empty() && display.resolutionIndex >= 0 && display.resolutionIndex < static_cast<int>(display.resolutions.size()))
    {
        std::string label = DisplaySettingsController::formatResolution(display.resolutions[display.resolutionIndex]);
        ImGui::BeginDisabled(display.modeIndex != 1);
        if (ImGui::BeginCombo("Resolution", label.c_str()))
        {
            for (size_t i = 0; i < display.resolutions.size(); ++i)
            {
                bool selected = static_cast<int>(i) == display.resolutionIndex;
                std::string option = DisplaySettingsController::formatResolution(display.resolutions[i]);
                if (ImGui::Selectable(option.c_str(), selected))
                {
                    display.resolutionIndex = static_cast<int>(i);
                    displayController_.markDirty();
                    dirty_ = true;
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
    }

    if (!display.refreshRates.empty() && display.refreshIndex >= 0 && display.refreshIndex < static_cast<int>(display.refreshRates.size()))
    {
        int currentHz = display.refreshRates[display.refreshIndex];
        std::string label = std::to_string(currentHz) + " Hz";
        if (ImGui::BeginCombo("Refresh rate", label.c_str()))
        {
            for (size_t i = 0; i < display.refreshRates.size(); ++i)
            {
                bool selected = static_cast<int>(i) == display.refreshIndex;
                std::string option = std::to_string(display.refreshRates[i]) + " Hz";
                if (ImGui::Selectable(option.c_str(), selected))
                {
                    display.refreshIndex = static_cast<int>(i);
                    displayController_.markDirty();
                    dirty_ = true;
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("HDR & color");
    ImGui::TextColored(kMuted, "Connect to platform APIs later.");
}

void SettingsScene::drawDeviceTab()
{
    ImGui::TextDisabled("Audio devices");
    const auto optionsIn = DeviceSelector::makeOptions(audioSession_.devices(), DeviceRole::Input, audioSession_.selectedInputDevice());
    if (DeviceSelector::combo("Input device", optionsIn, DeviceRole::Input, [&](unsigned int id)
                              { audioSession_.selectInputDevice(id); }))
    {
        dirty_ = true;
    }

    const auto optionsOut = DeviceSelector::makeOptions(audioSession_.devices(), DeviceRole::Output, audioSession_.selectedOutputDevice());
    if (DeviceSelector::combo("Output device", optionsOut, DeviceRole::Output, [&](unsigned int id)
                              { audioSession_.selectOutputDevice(id); }))
    {
        dirty_ = true;
    }

    if (ImGui::Checkbox("Hotplug notifications", &devices_.hotplugNotifications))
    {
        dirty_ = true;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Controllers");
    ImGui::TextColored(kMuted, "Gamepads, MIDI, and other controllers can be configured here.");
}

void SettingsScene::drawGameplayTab()
{
    if (ImGui::Checkbox("Metronome enabled", &gameplay_.metronomeEnabled))
    {
        dirty_ = true;
    }
    ImGui::BeginDisabled(!gameplay_.metronomeEnabled);
    if (ImGui::SliderFloat("Metronome volume", &gameplay_.metronomeVolume, 0.0f, 1.0f, "%.2f"))
    {
        dirty_ = true;
    }
    if (ImGui::Checkbox("Visual metronome", &gameplay_.visualMetronome))
    {
        dirty_ = true;
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    if (ImGui::Checkbox("Show background tips", &gameplay_.backgroundTips))
    {
        dirty_ = true;
    }
    if (ImGui::SliderInt("Note highway speed", &gameplay_.noteSpeed, 1, 10))
    {
        dirty_ = true;
    }
    if (ImGui::SliderInt("Hit window leniency", &gameplay_.hitWindow, 0, 4))
    {
        dirty_ = true;
    }
    ImGui::TextColored(kMuted, "Configure rhythm readability: speed, feedback, and timing window.");
}

void SettingsScene::drawFooter(GraphicsContext &gfx)
{
    const bool hasPendingChanges = dirty_ || displayController_.dirty();
    ImVec2 fullWidth(ImGui::GetContentRegionAvail().x, 0.0f);
    ImGui::BeginDisabled(!hasPendingChanges);
    if (ui_.button("Apply changes", fullWidth))
    {
        applySettings(gfx);
    }
    ImGui::EndDisabled();
    ImGui::Dummy(ImVec2(0, 6));
    if (ui_.button("Back to menu", fullWidth))
    {
        if (hasPendingChanges)
        {
            unsavedModal_.open();
        }
        else
        {
            finished_ = true;
        }
    }
}

void SettingsScene::applyAudio()
{
    if (audioSettings_.enableInputMonitor)
    {
        if (!audioSession_.monitoring())
        {
            audioSession_.startMonitoring();
        }
    }
    else
    {
        if (audioSession_.monitoring())
        {
            audioSession_.stopMonitoring(false);
        }
    }
}

void SettingsScene::applySettings(GraphicsContext &gfx)
{
    displayController_.apply(gfx);
    applyAudio();
    dirty_ = false;
}

void SettingsScene::render(float /*dt*/, const FrameInput & /*input*/, GraphicsContext &gfx, std::atomic<bool> & /*quitFlag*/)
{
    lastGfx_ = &gfx;
    const std::array<ImVec4, 4> bg = {
        ImVec4(0.05f, 0.07f, 0.11f, 1.0f),
        ImVec4(0.06f, 0.09f, 0.15f, 1.0f),
        ImVec4(0.04f, 0.05f, 0.09f, 1.0f),
        ImVec4(0.08f, 0.10f, 0.14f, 1.0f)};
    PanelLayout panel = UILayout::beginFullscreen("Settings", "settings_panel", 32.0f, bg);

    if (panel.windowOpen)
    {
        ImGui::BeginChild("settings_panel", panel.contentSize, false);

        drawHeader();

        if (ImGui::BeginTabBar("settings_tabs"))
        {
            if (ImGui::BeginTabItem("Audio"))
            {
                drawAudioTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Graphics"))
            {
                drawGraphicsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Display"))
            {
                drawDisplayTab(gfx);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Devices"))
            {
                drawDeviceTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Gameplay"))
            {
                drawGameplayTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        drawFooter(gfx);

        ImGui::EndChild();
    }
    UILayout::endFullscreen();

    if (auto choice = unsavedModal_.draw(ImVec2(360.0f, 0.0f)))
    {
        if (*choice == 0)
        {
            if (lastGfx_)
            {
                applySettings(*lastGfx_);
            }
            finished_ = true;
        }
        else if (*choice == 1)
        {
            dirty_ = false;
            displayController_.clearDirty();
            finished_ = true;
        }
        // choice 2 = Stay, no action.
    }
}
