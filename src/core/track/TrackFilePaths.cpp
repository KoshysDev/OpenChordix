#include "track/TrackFilePaths.h"

#include <utility>

namespace openchordix::track
{
    namespace
    {
        constexpr const char *kSongsDirectoryName = "Songs";
        constexpr const char *kDefaultChartFileName = "chart.ocx";
    }

    std::filesystem::path resolveTrackRootDirectory(std::filesystem::path rootDir)
    {
        if (rootDir.empty())
        {
            return std::filesystem::current_path();
        }
        return rootDir;
    }

    std::filesystem::path resolveSongDirectory(const TrackInfo &track, const std::filesystem::path &rootDir)
    {
        const std::filesystem::path resolvedRoot = resolveTrackRootDirectory(rootDir);
        std::filesystem::path songDir(track.directory);
        if (songDir.empty())
        {
            return resolvedRoot / kSongsDirectoryName / track.id;
        }
        if (songDir.is_relative())
        {
            return resolvedRoot / songDir;
        }
        return songDir;
    }

    std::filesystem::path resolveChartPath(const TrackInfo &track, const std::filesystem::path &rootDir)
    {
        std::filesystem::path chartPath(track.chartFile);
        if (chartPath.empty())
        {
            chartPath = kDefaultChartFileName;
        }
        if (chartPath.is_relative())
        {
            return resolveSongDirectory(track, rootDir) / chartPath;
        }
        return chartPath;
    }

    std::filesystem::path resolveAudioPath(const TrackInfo &track, const std::filesystem::path &rootDir)
    {
        std::filesystem::path audioPath(track.audioFile);
        if (audioPath.empty())
        {
            return {};
        }
        if (audioPath.is_relative())
        {
            return resolveSongDirectory(track, rootDir) / audioPath;
        }
        return audioPath;
    }
}
