#include "TunerScene.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    const ImVec4 kAccent = ImVec4(0.30f, 0.78f, 0.96f, 1.0f);
    const ImVec4 kMuted = ImVec4(0.72f, 0.78f, 0.88f, 1.0f);

    int inferMidiFromPitchClass(std::string_view note, int previousMidi)
    {
        const std::string targetLabel = openchordix::track::displayTuningNote(note);
        int bestMidi = -1;
        int bestDistance = 999;

        for (int midi = 0; midi <= 127; ++midi)
        {
            if (openchordix::track::displayTuningNote(openchordix::track::tuningNoteLabelFromMidi(midi)) != targetLabel)
            {
                continue;
            }

            if (previousMidi < 0)
            {
                if (midi >= 36 && midi <= 76)
                {
                    bestMidi = std::max(bestMidi, midi);
                }
                continue;
            }

            if (midi >= previousMidi)
            {
                continue;
            }

            const int distance = previousMidi - midi;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestMidi = midi;
            }
        }

        if (bestMidi >= 0)
        {
            return bestMidi;
        }

        for (int midi = 0; midi <= 127; ++midi)
        {
            if (openchordix::track::displayTuningNote(openchordix::track::tuningNoteLabelFromMidi(midi)) == targetLabel)
            {
                return midi;
            }
        }
        return -1;
    }

}

TunerScene::TunerScene(AudioSession &audio, AnimatedUI &ui)
    : audio_(audio), ui_(ui)
{
    openchordix::track::TuningLibrary tuningLibrary;
    auto makeTargets = [](const std::vector<std::string> &notes)
    {
        std::vector<StringTarget> targets;
        targets.reserve(notes.size());
        int previousMidi = -1;
        for (const std::string &note : notes)
        {
            int midi = openchordix::track::tuningNoteMidi(note).value_or(-1);
            if (midi < 0)
            {
                midi = inferMidiFromPitchClass(note, previousMidi);
            }

            StringTarget target;
            target.midi = midi;
            target.frequency = midi >= 0 ? static_cast<float>(openchordix::track::tuningFrequencyFromMidi(midi)) : 0.0f;
            target.label = midi >= 0 ? openchordix::track::tuningNoteLabelFromMidi(midi) : openchordix::track::displayTuningNote(note);
            targets.push_back(std::move(target));
            if (midi >= 0)
            {
                previousMidi = midi;
            }
        }
        return targets;
    };
    for (const auto &preset : tuningLibrary.presets())
    {
        tunings_.push_back(TuningProfile{
            preset.name,
            openchordix::track::tuningNotesSummary(preset.notes),
            makeTargets(preset.notes)});
    }

    if (tunings_.empty())
    {
        const std::vector<std::string> fallbackNotes = {"E4", "B3", "G3", "D3", "A2", "E2"};
        tunings_.push_back(TuningProfile{
            "Standard E",
            openchordix::track::tuningNotesSummary(fallbackNotes),
            makeTargets(fallbackNotes)});
    }

    if (!tunings_.empty())
    {
        selectedString_ = static_cast<int>(tunings_.front().strings.size()) - 1;
        lastAutoString_ = selectedString_;
    }
}

int TunerScene::detectStringFromPitch(const PitchState &pitch) const
{
    if (!pitch.note.isValid || pitch.note.midiNoteNumber < 0)
    {
        return -1;
    }

    const auto &tuning = tunings_[tuningIndex_];
    float best = 999.0f;
    int bestIndex = -1;
    for (size_t i = 0; i < tuning.strings.size(); ++i)
    {
        float diff = std::abs(static_cast<float>(tuning.strings[i].midi - pitch.note.midiNoteNumber));
        if (diff < best)
        {
            best = diff;
            bestIndex = static_cast<int>(i);
        }
    }
    return bestIndex;
}

int TunerScene::activeStringIndex(const PitchState &pitch)
{
    if (!autoDetectString_)
    {
        return selectedString_;
    }

    int detected = detectStringFromPitch(pitch);
    if (detected >= 0)
    {
        lastAutoString_ = detected;
    }
    return lastAutoString_;
}

void TunerScene::drawTuningSelector()
{
    ImGui::SeparatorText("Tunings");
    ImGui::BeginChild("tuning_list", ImVec2(0, 190), true);
    for (size_t i = 0; i < tunings_.size(); ++i)
    {
        const bool selected = tuningIndex_ == i;
        if (ImGui::Selectable(tunings_[i].name.c_str(), selected))
        {
            tuningIndex_ = i;
            selectedString_ = static_cast<int>(tunings_[i].strings.size()) - 1;
            lastAutoString_ = selectedString_;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", tunings_[i].subtitle.c_str());
    }
    ImGui::EndChild();
}

void TunerScene::drawStringSelector()
{
    const auto &tuning = tunings_[tuningIndex_];

    ImGui::SeparatorText("String focus");
    ImGui::TextDisabled("Auto picks the closest string to what you are playing.");
    ImGui::Checkbox("Auto string detection", &autoDetectString_);
    if (autoDetectString_)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(manual selection locked)");
    }

    ImGui::BeginDisabled(autoDetectString_);
    ImGui::BeginChild("string_buttons", ImVec2(0, 80), true);
    for (size_t i = 0; i < tuning.strings.size(); ++i)
    {
        const bool active = selectedString_ == static_cast<int>(i);
        ImVec2 btnSize = ImVec2(ImGui::GetContentRegionAvail().x / 3.0f - 6.0f, 32.0f);
        if (btnSize.x < 100.0f)
        {
            btnSize.x = 100.0f;
        }

        if (ImGui::Button(tuning.strings[i].label.c_str(), btnSize))
        {
            selectedString_ = static_cast<int>(i);
        }
        if ((i + 1) % 3 != 0)
        {
            ImGui::SameLine();
        }
    }
    ImGui::EndChild();
    ImGui::EndDisabled();
}

void TunerScene::drawLivePanel(const PitchState &pitch, const StringTarget &target, int stringIndex)
{
    ImGui::SeparatorText("Live tuner");

    bool hasPitch = audio_.monitoring() && pitch.note.isValid && pitch.frequency > 0.0f;
    float centsDiff = 0.0f;
    if (hasPitch && target.frequency > 0.0f)
    {
        centsDiff = 1200.0f * std::log2(pitch.frequency / target.frequency);
    }

    bool inTune = hasPitch && std::abs(centsDiff) < 4.0f;
    float clamped = std::clamp(centsDiff, -50.0f, 50.0f);
    float normalized = (clamped + 50.0f) / 100.0f;

    ImGui::Text("Tuning %s string", target.label.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%s", autoDetectString_ ? "(auto)" : "(manual)");

    ImGui::TextDisabled("Target");
    ImGui::SameLine();
    ImGui::Text("%s \xe2\x80\xa2 %.1f Hz", target.label.c_str(), target.frequency);

    ImGui::TextDisabled("You are playing");
    if (hasPitch)
    {
        ImGui::SameLine();
        ImGui::Text("%s%d @ %.1f Hz", pitch.note.name.c_str(), pitch.note.octave, pitch.frequency);
    }
    else
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Play a string to lock on.");
    }

    ImVec2 meterSize(ImGui::GetContentRegionAvail().x, 90.0f);
    ImGui::InvisibleButton("cents_meter", meterSize);
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    const float radius = 10.0f;
    dl->AddRectFilled(min, max, ImGui::GetColorU32(ImVec4(0.10f, 0.12f, 0.17f, 1.0f)), radius);
    dl->AddRect(min, max, ImGui::GetColorU32(ImVec4(0.22f, 0.28f, 0.36f, 1.0f)), radius, 0, 2.0f);

    float centerX = (min.x + max.x) * 0.5f;
    dl->AddLine(ImVec2(centerX, min.y), ImVec2(centerX, max.y), ImGui::GetColorU32(ImVec4(0.32f, 0.42f, 0.54f, 0.8f)), 2.0f);

    float cursorX = min.x + normalized * (max.x - min.x);
    ImU32 cursorColor = ImGui::GetColorU32(inTune ? ImVec4(0.33f, 0.84f, 0.54f, 1.0f) : kAccent);
    dl->AddTriangleFilled(ImVec2(cursorX, min.y + 16.0f),
                          ImVec2(cursorX - 10.0f, min.y + 36.0f),
                          ImVec2(cursorX + 10.0f, min.y + 36.0f),
                          cursorColor);
    dl->AddLine(ImVec2(cursorX, min.y + 36.0f), ImVec2(cursorX, max.y - 8.0f), cursorColor, 3.0f);

    dl->AddText(ImVec2(min.x + 12.0f, max.y - 24.0f), ImGui::GetColorU32(kMuted), "-50c");
    ImVec2 textSize = ImGui::CalcTextSize("+50c");
    dl->AddText(ImVec2(max.x - textSize.x - 12.0f, max.y - 24.0f), ImGui::GetColorU32(kMuted), "+50c");

    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    const char *direction = "Play a string";
    ImVec4 directionColor = ImVec4(0.86f, 0.62f, 0.44f, 1.0f);
    if (hasPitch)
    {
        if (inTune)
        {
            direction = "In tune";
            directionColor = ImVec4(0.44f, 0.90f, 0.64f, 1.0f);
        }
        else if (centsDiff < 0.0f)
        {
            direction = "Tune up";
            directionColor = ImVec4(0.36f, 0.72f, 1.0f, 1.0f);
        }
        else
        {
            direction = "Tune down";
            directionColor = ImVec4(0.93f, 0.52f, 0.46f, 1.0f);
        }
    }

    ImGui::TextColored(directionColor, "%s", direction);
    if (hasPitch && !inTune)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(%.1f cents)", std::abs(centsDiff));
    }

    ImGui::Spacing();
    ImGui::TextDisabled("String %d: %s", static_cast<int>(stringIndex + 1), target.label.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("\xe2\x80\xa2 Target %.1f Hz", target.frequency);
}

void TunerScene::render(float /*dt*/, const FrameInput & /*input*/, GraphicsContext & /*gfx*/, std::atomic<bool> &quitFlag)
{
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("Tuner", nullptr, flags))
    {
        ImDrawList *bg = ImGui::GetBackgroundDrawList();
        bg->AddRectFilledMultiColor(ImVec2(0, 0), screen,
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.06f, 0.10f, 1.0f)),
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f, 0.09f, 0.14f, 1.0f)),
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.05f, 0.09f, 1.0f)),
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.10f, 0.13f, 1.0f)));

        const float margin = 36.0f;
        ImGui::SetCursorPos(ImVec2(margin, margin));
        ImVec2 panelSize = ImVec2(screen.x - margin * 2.0f, screen.y - margin * 2.0f);
        ImGui::BeginChild("tuner_panel", panelSize, false);

        ui_.heading("Guitar tuner", kAccent);
        ImGui::Text("Pick a tuning profile.");
        ImGui::Spacing();

        PitchState pitch = audio_.pitch();
        const auto &tuning = tunings_[tuningIndex_];
        int stringIndex = activeStringIndex(pitch);
        stringIndex = std::clamp(stringIndex, 0, static_cast<int>(tuning.strings.size()) - 1);
        const StringTarget &target = tuning.strings[static_cast<size_t>(stringIndex)];

        if (ImGui::BeginTable("tuner_layout", 2, ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.42f);
            ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch, 0.58f);

            ImGui::TableNextColumn();
            drawTuningSelector();
            drawStringSelector();
            ImGui::Spacing();
            ImVec2 fullWidth(ImGui::GetContentRegionAvail().x, 0.0f);
            if (ui_.button("Back to menu", fullWidth))
            {
                finished_ = true;
            }
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TableNextColumn();
            if (!audio_.monitoring())
            {
                ImGui::TextColored(ImVec4(0.94f, 0.71f, 0.44f, 1.0f), "Monitoring is off.");
                ImGui::TextDisabled("Start monitoring from Audio Setup to use the tuner.");
                ImGui::Spacing();
            }
            drawLivePanel(pitch, target, stringIndex);

            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
    ImGui::End();
}
