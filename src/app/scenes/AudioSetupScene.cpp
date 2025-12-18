#include "AudioSetupScene.h"

#include <imgui/imgui.h>
#include <sstream>
#include <algorithm>
#include <array>

#include "ui/DeviceSelector.h"
#include "ui/UILayout.h"

AudioSetupScene::AudioSetupScene(AudioSession &audio, NoteConverter &noteConverter, AnimatedUI &ui)
    : audio_(audio), noteConverter_(noteConverter), ui_(ui)
{
}

void AudioSetupScene::drawInputDeviceList()
{
    auto options = DeviceSelector::makeOptions(audio_.devices(), DeviceRole::Input, audio_.selectedInputDevice());
    DeviceSelector::list("input_device_list", options, DeviceRole::Input, [&](unsigned int id)
                         {
                             audio_.selectInputDevice(id);
                             audio_.stopMonitoring(false); }, ImVec2(0, 230));
}

void AudioSetupScene::drawOutputDeviceList()
{
    auto options = DeviceSelector::makeOptions(audio_.devices(), DeviceRole::Output, audio_.selectedOutputDevice());
    DeviceSelector::list("output_device_list", options, DeviceRole::Output, [&](unsigned int id)
                         {
                             audio_.selectOutputDevice(id);
                             audio_.stopMonitoring(false); }, ImVec2(0, 230));
}

void AudioSetupScene::render(float dt, const FrameInput & /*input*/, GraphicsContext &gfx, std::atomic<bool> & /*quitFlag*/)
{
    audio_.updatePitch(noteConverter_);

    const ImVec4 accent = ImVec4(0.35f, 0.73f, 0.98f, 1.0f);
    const std::array<ImVec4, 4> bg = {
        ImVec4(0.06f, 0.07f, 0.10f, 1.0f),
        ImVec4(0.08f, 0.10f, 0.14f, 1.0f),
        ImVec4(0.05f, 0.05f, 0.08f, 1.0f),
        ImVec4(0.04f, 0.05f, 0.07f, 1.0f)};

    PanelLayout panel = UILayout::beginFullscreen("OpenChordix Setup", "content_panel", 36.0f, bg);
    if (panel.windowOpen)
    {
        ImGui::BeginChild("content_panel", panel.contentSize, false);

        ui_.heading("Routing setup", accent);
        ImGui::Text("Select audio device like your audio interface to monitor and use guitar input in game.");
        ImGui::Spacing();

        ImGui::SeparatorText("Audio API");
        std::string apiLabel = RtAudio::getApiDisplayName(audio_.api());
        if (ImGui::BeginCombo("##api", apiLabel.c_str()))
        {
            for (RtAudio::Api api : {RtAudio::Api::LINUX_ALSA, RtAudio::Api::LINUX_PULSE, RtAudio::Api::UNIX_JACK, RtAudio::Api::UNSPECIFIED})
            {
                bool isSelected = api == audio_.api();
                std::string label = RtAudio::getApiDisplayName(api);
                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    if (audio_.api() != api)
                    {
                        audio_.setApi(api);
                        audio_.refreshDevices(api);
                    }
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Pick audio API.");

        ImGui::SeparatorText("Input Device");
        drawInputDeviceList();

        ImGui::SeparatorText("Output Device");
        ImGui::TextDisabled("Pick where the monitored signal should be sent.");
        drawOutputDeviceList();

        int sr = static_cast<int>(audio_.sampleRate());
        std::string srLabel = std::to_string(sr) + " Hz";
        if (ImGui::BeginCombo("Sample Rate", srLabel.c_str()))
        {
            for (int candidate : audio_.allowedSampleRates())
            {
                bool selected = candidate == sr;
                std::string label = std::to_string(candidate) + " Hz";
                if (ImGui::Selectable(label.c_str(), selected))
                {
                    audio_.setSampleRate(static_cast<unsigned int>(candidate));
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        int buf = static_cast<int>(audio_.bufferFrames());
        std::string bufLabel = std::to_string(buf) + " frames";
        if (ImGui::BeginCombo("Buffer Size", bufLabel.c_str()))
        {
            for (int candidate : audio_.allowedBufferSizes())
            {
                bool selected = candidate == buf;
                std::string label = std::to_string(candidate) + " frames";
                if (ImGui::Selectable(label.c_str(), selected))
                {
                    audio_.setBufferFrames(static_cast<unsigned int>(candidate));
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::TextDisabled("JACK lets the server pick the buffer automatically.");

        ImVec2 fullWidth(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ui_.button("Start monitoring", ImVec2(fullWidth.x * 0.65f, 0.0f)))
        {
            audio_.stopMonitoring(false);
            audio_.startMonitoring();
        }
        ImGui::SameLine();
        if (ui_.button("Stop", ImVec2(fullWidth.x * 0.33f, 0.0f)))
        {
            audio_.stopMonitoring(true);
        }

        if (!audio_.status().empty())
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.90f, 0.73f, 0.39f, 1.0f), "%s", audio_.status().c_str());
        }

        PitchState currentPitch = audio_.pitch();

        ImGui::SeparatorText("Live Pitch");
        if (audio_.monitoring() && currentPitch.frequency > 0.0f && currentPitch.note.isValid)
        {
            ImGui::Text("Frequency: %.1f Hz", currentPitch.frequency);
            ImGui::Text("Note: %s%d", currentPitch.note.name.c_str(), currentPitch.note.octave);
            float centsNorm = std::clamp((currentPitch.note.cents + 50.0f) / 100.0f, 0.0f, 1.0f);
            ImGui::ProgressBar(centsNorm, ImVec2(-1.0f, 0.0f), "cents offset");
        }
        else
        {
            ImGui::TextDisabled("Waiting for a stable pitch...");
        }

        ImGui::Spacing();
        if (ui_.button("Continue to menu", ImVec2(fullWidth.x, 0.0f)))
        {
            finished_ = true;
        }

        ImGui::EndChild();
    }
    UILayout::endFullscreen();
}
