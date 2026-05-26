#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "track/import/TrackImport.h"

namespace openchordix::track::imports
{
    struct ImportTimingNoteDebug
    {
        int measureIndex = -1;
        int tickWithinMeasure = 0;
        int absoluteTick = 0;
        int durationTicks = 0;
        int stringIndex = -1;
        int fret = -1;
        std::string techniques;
    };

    struct ImportTimingPartDebug
    {
        std::string name;
        std::optional<int> firstNoteTick;
        int firstNoteMeasureIndex = -1;
        int firstNonEmptyMeasureIndex = -1;
        std::vector<ImportTimingNoteDebug> notes;
    };

    struct ImportTimingDebugSummary
    {
        int ticksPerBeat = 0;
        std::optional<double> initialTempo;
        std::size_t measureCount = 0;
        std::vector<ImportedMeasure> measures;
        std::vector<ImportTimingPartDebug> parts;
    };

    ImportTimingDebugSummary buildTimingDebugSummary(const ImportedSong &song,
                                                      std::size_t maximumNotesPerPart = 16,
                                                      std::size_t maximumMeasures = 32);
}
