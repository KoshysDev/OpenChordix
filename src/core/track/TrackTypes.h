#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace openchordix::track
{
    inline constexpr int kDefaultTrackStrings = 6;
    inline constexpr int kMaxTrackStrings = 12;

    inline int clampTrackStringCount(int value)
    {
        return std::clamp(value, 1, kMaxTrackStrings);
    }

    inline bool isBassLikePartName(std::string_view name)
    {
        std::string lowered(name);
        std::transform(
            lowered.begin(),
            lowered.end(),
            lowered.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
        return lowered.find("bass") != std::string::npos;
    }

    inline std::vector<std::string> defaultTuningForPart(std::string_view partName, int stringCount)
    {
        static constexpr std::array<const char *, kMaxTrackStrings> kGuitarTuning = {
            "e", "B", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#"};
        static constexpr std::array<const char *, kMaxTrackStrings> kBassTuning = {
            "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#", "F", "C"};

        const auto &defaults = isBassLikePartName(partName) ? kBassTuning : kGuitarTuning;
        const int safeStringCount = clampTrackStringCount(stringCount);
        std::vector<std::string> tuning;
        tuning.reserve(static_cast<size_t>(safeStringCount));
        for (int i = 0; i < safeStringCount; ++i)
        {
            tuning.emplace_back(defaults[static_cast<size_t>(i)]);
        }
        return tuning;
    }

    inline std::vector<std::string> normalizeTrackTuning(const std::vector<std::string> &source,
                                                         std::string_view partName,
                                                         int stringCount)
    {
        const int safeStringCount = clampTrackStringCount(stringCount);
        std::vector<std::string> tuning;
        tuning.reserve(static_cast<size_t>(safeStringCount));
        for (const std::string &entry : source)
        {
            if (static_cast<int>(tuning.size()) >= safeStringCount)
            {
                break;
            }

            size_t first = 0;
            while (first < entry.size() && std::isspace(static_cast<unsigned char>(entry[first])) != 0)
            {
                ++first;
            }
            size_t last = entry.size();
            while (last > first && std::isspace(static_cast<unsigned char>(entry[last - 1])) != 0)
            {
                --last;
            }

            const std::string trimmed = entry.substr(first, last - first);
            tuning.push_back(trimmed);
        }

        const std::vector<std::string> defaults = defaultTuningForPart(partName, safeStringCount);
        while (static_cast<int>(tuning.size()) < safeStringCount)
        {
            tuning.push_back(defaults[tuning.size()]);
        }
        for (int i = 0; i < safeStringCount; ++i)
        {
            if (tuning[static_cast<size_t>(i)].empty())
            {
                tuning[static_cast<size_t>(i)] = defaults[static_cast<size_t>(i)];
            }
        }
        return tuning;
    }
}

struct TrackPart
{
    std::string name;
    int stringCount = openchordix::track::kDefaultTrackStrings;
    std::vector<std::string> tuning = openchordix::track::defaultTuningForPart("", openchordix::track::kDefaultTrackStrings);
};

struct TrackInfo
{
    std::string id;
    std::string title;
    std::string artist;
    std::string source;
    std::string mapper;
    int bpm = 0;
    std::string length;
    double previewStartSeconds = 0.0;
    std::string directory;
    std::string audioFile;
    std::string chartFile;
    std::vector<TrackPart> parts;
};
