#pragma once

#include <string>
#include <vector>

#include "track/TrackTypes.h"

class TrackCatalog
{
public:
    virtual ~TrackCatalog() = default;

    virtual const std::vector<TrackInfo> &tracks() const = 0;
    virtual std::vector<int> filter(const std::string &query) const = 0;
};
