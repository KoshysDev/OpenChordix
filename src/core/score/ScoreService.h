#pragma once

#include <string>
#include <vector>

#include "score/ScoreTypes.h"
#include "track/TrackTypes.h"

class TrackScoreService
{
public:
    virtual ~TrackScoreService() = default;

    virtual std::vector<ScoreEntry> scoresFor(const TrackInfo &track,
                                              const std::string &partName,
                                              ScoreCategory category) const = 0;
};
