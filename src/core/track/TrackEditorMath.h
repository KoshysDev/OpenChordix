#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>

namespace openchordix::track::editor
{
    inline constexpr std::array<int, 4> kSnapDivisors = {1, 2, 4, 8};

    inline std::optional<int> parseNonNegativeInteger(std::string_view value)
    {
        if (value.empty())
        {
            return std::nullopt;
        }

        int parsed = 0;
        for (const char ch : value)
        {
            if (ch < '0' || ch > '9')
            {
                return std::nullopt;
            }
            parsed = parsed * 10 + (ch - '0');
        }
        return parsed;
    }

    inline int parseLengthSeconds(std::string_view value)
    {
        const size_t separator = value.find(':');
        if (separator == std::string_view::npos)
        {
            return 0;
        }

        const auto minutes = parseNonNegativeInteger(value.substr(0, separator));
        const auto seconds = parseNonNegativeInteger(value.substr(separator + 1));
        if (!minutes.has_value() || !seconds.has_value())
        {
            return 0;
        }
        return std::max(0, *minutes * 60 + *seconds);
    }

    inline std::string formatClock(double seconds)
    {
        const int clamped = std::max(0, static_cast<int>(std::round(seconds)));
        const int minutes = clamped / 60;
        const int remainder = clamped % 60;
        std::string formatted;
        if (minutes < 10)
        {
            formatted.push_back('0');
        }
        formatted += std::to_string(minutes);
        formatted.push_back(':');
        if (remainder < 10)
        {
            formatted.push_back('0');
        }
        formatted += std::to_string(remainder);
        return formatted;
    }

    inline int snapTickSize(int ticksPerBeat, int snapIndex)
    {
        const int safeTicksPerBeat = std::max(1, ticksPerBeat);
        const int divisor = kSnapDivisors[std::clamp(snapIndex, 0, static_cast<int>(kSnapDivisors.size()) - 1)];
        return std::max(1, safeTicksPerBeat / divisor);
    }

    inline int quantizeTick(int tick, int ticksPerBeat, int snapIndex)
    {
        const int snap = snapTickSize(ticksPerBeat, snapIndex);
        return std::max(0, static_cast<int>(std::round(static_cast<float>(tick) / static_cast<float>(snap))) * snap);
    }

    inline int timelineTotalBeats(int durationSeconds, int bpm, int beatsPerMeasure)
    {
        const int safeDuration = std::max(0, durationSeconds);
        const int safeBpm = std::max(1, bpm);
        const int safeBeatsPerMeasure = std::max(1, beatsPerMeasure);
        return std::max(safeBeatsPerMeasure * 4,
                        static_cast<int>(std::ceil(safeDuration * static_cast<double>(safeBpm) / 60.0)));
    }

    inline int timelineTotalTicks(int durationSeconds, int bpm, int beatsPerMeasure, int ticksPerBeat)
    {
        const int safeTicksPerBeat = std::max(1, ticksPerBeat);
        return std::max(safeTicksPerBeat, timelineTotalBeats(durationSeconds, bpm, beatsPerMeasure) * safeTicksPerBeat);
    }

    inline double clampTransportCursor(double seconds, int songDurationSeconds)
    {
        return std::clamp(seconds, 0.0, static_cast<double>(std::max(0, songDurationSeconds)));
    }

    inline double timelineTickFromSeconds(double seconds, int bpm, int ticksPerBeat, int songDurationSeconds)
    {
        return clampTransportCursor(seconds, songDurationSeconds) * static_cast<double>(std::max(1, bpm)) / 60.0 *
               static_cast<double>(std::max(1, ticksPerBeat));
    }

    inline double timelineSecondsFromTick(double tick, int bpm, int ticksPerBeat)
    {
        return std::max(0.0, tick) / static_cast<double>(std::max(1, ticksPerBeat)) *
               60.0 / static_cast<double>(std::max(1, bpm));
    }
}
