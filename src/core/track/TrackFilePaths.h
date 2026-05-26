#pragma once

#include <filesystem>

#include "track/TrackTypes.h"

namespace openchordix::track
{
    std::filesystem::path resolveTrackRootDirectory(std::filesystem::path rootDir);
    std::filesystem::path resolveSongDirectory(const TrackInfo &track, const std::filesystem::path &rootDir);
    std::filesystem::path resolveChartPath(const TrackInfo &track, const std::filesystem::path &rootDir);
    std::filesystem::path resolveAudioPath(const TrackInfo &track, const std::filesystem::path &rootDir);
}
