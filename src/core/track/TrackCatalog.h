#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "track/TrackTypes.h"

class TrackCatalog
{
public:
    virtual ~TrackCatalog() = default;

    virtual const std::vector<TrackInfo> &tracks() const = 0;
    virtual std::vector<int> filter(const std::string &query) const = 0;
    virtual bool reload() = 0;
    virtual bool addTrack(const TrackInfo &track) = 0;
    virtual bool updateTrack(std::string_view id, const TrackInfo &track) = 0;
    virtual bool removeTrack(std::string_view id) = 0;
    virtual const std::filesystem::path &storagePath() const = 0;
};
