#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <imgui/imgui.h>

#include "track/TrackEditorMath.h"
#include "track/TrackStringUtils.h"
#include "track/TuningLibrary.h"

namespace track_editor
{
    inline const ImVec4 kAccent = ImVec4(0.34f, 0.78f, 0.98f, 1.0f);
    inline const ImVec4 kAccentSoft = ImVec4(0.22f, 0.46f, 0.74f, 0.92f);
    inline const ImVec4 kMuted = ImVec4(0.72f, 0.80f, 0.90f, 1.0f);
    inline const ImVec4 kPanel = ImVec4(0.07f, 0.10f, 0.14f, 0.96f);
    inline const ImVec4 kPanelSoft = ImVec4(0.09f, 0.13f, 0.18f, 0.94f);
    inline const ImVec4 kBorder = ImVec4(0.20f, 0.26f, 0.34f, 0.95f);
    inline const ImVec4 kGrid = ImVec4(0.19f, 0.23f, 0.30f, 1.0f);
    inline const ImVec4 kMeasure = ImVec4(0.34f, 0.78f, 0.98f, 0.52f);
    inline const ImVec4 kPlayhead = ImVec4(0.95f, 0.98f, 1.0f, 0.95f);
    inline const ImVec4 kPreviewMarker = ImVec4(0.44f, 0.90f, 1.0f, 1.0f);

    inline constexpr float kBottomBarHeight = 352.0f;
    inline constexpr float kLaneLabelWidth = 52.0f;
    inline constexpr float kRulerHeight = 34.0f;
    inline constexpr float kStatusStripHeight = 38.0f;
    inline constexpr float kEditorHorizontalMargin = 12.0f;
    inline constexpr float kEditorPanelSpacing = 8.0f;
    inline constexpr float kMinZoom = 0.55f;
    inline constexpr float kMaxZoom = 2.6f;

    inline constexpr std::array<const char *, 4> kSnapLabels = {"1/4", "1/8", "1/16", "1/32"};
    inline constexpr std::array<const char *, 4> kNoteTypeLabels = {"normal", "ghost", "dead", "tie"};
    inline constexpr std::array<const char *, 7> kSlideTypeLabels = {"none", "shift", "legato", "into_from_below", "into_from_above", "out_down", "out_up"};
    inline constexpr std::array<const char *, 8> kHarmonicTypeLabels = {"none", "natural", "artificial", "pinch", "tap", "semi", "feedback", "octave"};
    inline constexpr std::array<const char *, 4> kPluckStyleLabels = {"none", "tap", "slap", "pop"};

    using openchordix::track::editor::formatClock;
    using openchordix::track::editor::kSnapDivisors;
    using openchordix::track::editor::parseLengthSeconds;
    using openchordix::track::trimCopy;

    template <size_t N>
    void copyText(std::array<char, N> &buffer, std::string_view value)
    {
        if constexpr (N == 0)
        {
            return;
        }

        const size_t copyLength = std::min(value.size(), N - 1);
        if (copyLength > 0)
        {
            std::copy_n(value.begin(), copyLength, buffer.begin());
        }
        buffer[copyLength] = '\0';
        std::fill(buffer.begin() + static_cast<std::ptrdiff_t>(copyLength + 1), buffer.end(), '\0');
    }

    inline std::string defaultInstrumentName(size_t index)
    {
        return index == 0 ? "default" : "instrument " + std::to_string(index + 1);
    }

    inline void drawPanelFrame(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max, const ImVec4 &fillColor)
    {
        drawList->AddRectFilled(min, max, ImGui::GetColorU32(fillColor), 16.0f);
        drawList->AddRect(min, max, ImGui::GetColorU32(kBorder), 16.0f, 0, 1.2f);
    }

    inline const std::vector<std::string> &tuningNoteOptions()
    {
        static const std::vector<std::string> options = []
        {
            std::vector<std::string> labels;
            for (int midi = 23; midi <= 88; ++midi)
            {
                labels.push_back(openchordix::track::tuningNoteLabelFromMidi(midi));
            }
            return labels;
        }();
        return options;
    }
}
