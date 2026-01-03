#include "track/TrackCatalogMemory.h"

#include <algorithm>
#include <cctype>

TrackCatalogMemory::TrackCatalogMemory()
{
    tracks_.push_back(TrackInfo{
        "Master of Puppets",
        "Metallica",
        "Thrash Metal",
        "Raine",
        212,
        "8:36",
        {{"Lead Guitar"}, {"Rhythm Guitar"}, {"Bass"}}});

    tracks_.push_back(TrackInfo{
        "Back in Black",
        "AC/DC",
        "Rock",
        "Lux",
        94,
        "4:15",
        {{"Lead Guitar"}, {"Rhythm Guitar"}, {"Bass"}}});

    tracks_.push_back(TrackInfo{
        "Painkiller",
        "Judas Priest",
        "Heavy Metal",
        "Harper",
        188,
        "6:06",
        {{"Lead Guitar"}, {"Rhythm Guitar"}, {"Bass"}}});

    tracks_.push_back(TrackInfo{
        "Paranoid",
        "Black Sabbath",
        "Heavy metal",
        "Aster",
        163,
        "2:48",
        {{"Lead Guitar"}, {"Rhythm Guitar"}, {"Bass"}}});

    tracks_.push_back(TrackInfo{
        "Holy Wars... The Punishment Due",
        "Megadeth",
        "Thrash Metal",
        "Mara",
        178,
        "6:36",
        {{"Lead Guitar"}, {"Rhythm Guitar"}, {"Bass"}}});
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
