#pragma once

#include "track/TrackCatalog.h"

class TrackCatalogMemory : public TrackCatalog
{
public:
    TrackCatalogMemory();

    const std::vector<TrackInfo> &tracks() const override;
    std::vector<int> filter(const std::string &query) const override;

private:
    static std::string lowerCopy(const std::string &value);
    bool matchesFilter(const TrackInfo &track, const std::string &query) const;

    std::vector<TrackInfo> tracks_;
};
