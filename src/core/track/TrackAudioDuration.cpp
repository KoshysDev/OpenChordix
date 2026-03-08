#include "track/TrackAudioDuration.h"

#include <array>
#include <cmath>
#include <cstdio>

#include <aubio/aubio.h>

namespace openchordix::track
{
    std::optional<std::string> readAudioDuration(const std::filesystem::path &audioPath)
    {
        const std::string path = audioPath.string();
        if (path.empty())
        {
            return std::nullopt;
        }

        aubio_source_t *source = new_aubio_source(path.c_str(), 0, 512);
        if (source == nullptr)
        {
            return std::nullopt;
        }

        const uint_t samplerate = aubio_source_get_samplerate(source);
        const uint_t durationFrames = aubio_source_get_duration(source);
        del_aubio_source(source);

        if (samplerate == 0 || durationFrames == 0)
        {
            return std::nullopt;
        }

        const int totalSeconds = static_cast<int>(
            std::round(static_cast<double>(durationFrames) / static_cast<double>(samplerate)));
        const int minutes = totalSeconds / 60;
        const int seconds = totalSeconds % 60;

        std::array<char, 16> formatted{};
        std::snprintf(formatted.data(), formatted.size(), "%02d:%02d", minutes, seconds);
        return std::string(formatted.data());
    }
}

