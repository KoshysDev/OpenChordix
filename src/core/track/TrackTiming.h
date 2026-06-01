#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>

namespace openchordix::track
{
    inline bool isPowerOfTwoDenominator(int denominator)
    {
        return denominator > 0 && (denominator & (denominator - 1)) == 0;
    }

    inline std::optional<int> measureLengthTicks(int ticksPerBeat, int numerator, int denominator)
    {
        if (ticksPerBeat <= 0 || numerator <= 0 || denominator <= 0 ||
            !isPowerOfTwoDenominator(denominator))
        {
            return std::nullopt;
        }

        const std::int64_t wholeNoteTicks = static_cast<std::int64_t>(ticksPerBeat) * 4;
        const std::int64_t scaled = static_cast<std::int64_t>(numerator) * wholeNoteTicks;
        if (scaled <= 0 || scaled > static_cast<std::int64_t>(std::numeric_limits<int>::max()) * denominator)
        {
            return std::nullopt;
        }
        return static_cast<int>(std::max<std::int64_t>(1, scaled / denominator));
    }

    inline int beatOffsetTicksInMeasure(int ticksPerBeat, int beatIndex, int denominator)
    {
        const std::int64_t wholeNoteTicks = static_cast<std::int64_t>(std::max(1, ticksPerBeat)) * 4;
        const std::int64_t scaled = static_cast<std::int64_t>(std::max(0, beatIndex)) * wholeNoteTicks;
        return static_cast<int>(std::clamp<std::int64_t>(
            scaled / std::max(1, denominator),
            0,
            std::numeric_limits<int>::max()));
    }
}
