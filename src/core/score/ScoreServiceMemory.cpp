#include "score/ScoreServiceMemory.h"

std::vector<ScoreEntry> TrackScoreServiceMemory::scoresFor(const TrackInfo & /*track*/,
                                                           const std::string & /*partName*/,
                                                           ScoreCategory category) const
{
    switch (category)
    {
    case ScoreCategory::Local:
        return {
            {1, "Pix", 512, 982445},
            {2, "Juno", 478, 948210},
            {3, "Riff", 450, 900330}};
    case ScoreCategory::Online:
        return {
            {1, "Aria", 590, 1234900},
            {2, "Rune", 560, 1120540},
            {3, "Nova", 545, 1050100}};
    case ScoreCategory::Country:
        return {
            {1, "Echo", 520, 1010250},
            {2, "Slate", 498, 980880},
            {3, "Vale", 470, 960400}};
    }
    return {};
}
