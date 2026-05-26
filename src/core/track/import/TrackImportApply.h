#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "track/TrackChartDocument.h"
#include "track/import/TrackImport.h"

namespace openchordix::track::imports
{
    enum class ImportMergeMode
    {
        Additive,
        ReplaceAllNotes
    };

    enum class ImportPlacement
    {
        StartAtBeginning,
        CurrentCursor,
        AppendAfterExisting,
        CustomTickOffset
    };

    struct ImportPlacementOptions
    {
        ImportPlacement mode = ImportPlacement::StartAtBeginning;
        int currentCursorTick = 0;
        int customTickOffset = 0;
    };

    struct SelectedImportPart
    {
        std::size_t sourceIndex = 0;
        bool enabled = true;
        std::string name;
        int stringCount = openchordix::track::kDefaultTrackStrings;
        std::vector<std::string> tuning;
    };

    using ImportPartSelection = SelectedImportPart;

    struct ImportApplySummary
    {
        std::size_t addedNotes = 0;
        int placementOffsetTicks = 0;
        std::vector<TrackPart> appliedParts;
    };

    ImportApplySummary applyImportedParts(TrackChartDocument &document,
                                          const ImportedSong &song,
                                          const std::vector<SelectedImportPart> &selections,
                                          ImportMergeMode mergeMode = ImportMergeMode::Additive,
                                          const ImportPlacementOptions &placement = {});
}
