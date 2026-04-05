#include "track/TrackCatalogMemory.h"

#include <algorithm>
#include <cctype>
#include <utility>

TrackCatalogMemory::TrackCatalogMemory()
{
}

const std::vector<TrackInfo> &TrackCatalogMemory::tracks() const
{
    return tracks_;
}

std::vector<int> TrackCatalogMemory::filter(const std::string &query) const
{
    std::vector<int> out;
    std::string lowered = lowerCopy(query);
    for (size_t i = 0; i < tracks_.size(); ++i)
    {
        if (lowered.empty() || matchesFilter(tracks_[i], lowered))
        {
            out.push_back(static_cast<int>(i));
        }
    }
    return out;
}

bool TrackCatalogMemory::matchesFilter(const TrackInfo &track, const std::string &query) const
{
    std::string title = lowerCopy(track.title);
    std::string artist = lowerCopy(track.artist);
    return title.find(query) != std::string::npos || artist.find(query) != std::string::npos;
}

std::string TrackCatalogMemory::lowerCopy(const std::string &value)
{
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool TrackCatalogMemory::reload()
{
    return true;
}

bool TrackCatalogMemory::addTrack(const TrackInfo &track)
{
    tracks_.push_back(track);
    return true;
}

bool TrackCatalogMemory::updateTrack(std::string_view id, const TrackInfo &track)
{
    const auto it = std::find_if(
        tracks_.begin(),
        tracks_.end(),
        [&](const TrackInfo &existing)
        {
            return existing.id == id;
        });
    if (it == tracks_.end())
    {
        return false;
    }
    TrackInfo replacement = track;
    replacement.id = std::string(id);
    *it = std::move(replacement);
    return true;
}

bool TrackCatalogMemory::removeTrack(std::string_view id)
{
    const auto it = std::find_if(
        tracks_.begin(),
        tracks_.end(),
        [&](const TrackInfo &track)
        {
            return track.id == id;
        });
    if (it == tracks_.end())
    {
        return false;
    }
    tracks_.erase(it);
    return true;
}

const std::filesystem::path &TrackCatalogMemory::storagePath() const
{
    return storagePath_;
}
