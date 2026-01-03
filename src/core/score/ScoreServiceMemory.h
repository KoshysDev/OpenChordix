#pragma once

#include "score/ScoreService.h"

class TrackScoreServiceMemory : public TrackScoreService
{
public:
    std::vector<ScoreEntry> scoresFor(const TrackInfo &track,
                                      const std::string &partName,
                                      ScoreCategory category) const override;
};
