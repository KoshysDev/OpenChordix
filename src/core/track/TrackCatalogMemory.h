#pragma once

#include "track/TrackCatalog.h"

class TrackCatalogMemory : public TrackCatalog
{
public:
    TrackCatalogMemory();

    const std::vector<TrackInfo> &tracks() const override;
    std::vector<int> filter(const std::string &query) const override;
    bool reload() override;
    bool addTrack(const TrackInfo &track) override;
    bool updateTrack(std::string_view id, const TrackInfo &track) override;
    bool removeTrack(std::string_view id) override;
    const std::filesystem::path &storagePath() const override;

private:
    static std::string lowerCopy(const std::string &value);
    bool matchesFilter(const TrackInfo &track, const std::string &query) const;

    std::vector<TrackInfo> tracks_;
    std::filesystem::path storagePath_{"memory://track-catalog"};
};
