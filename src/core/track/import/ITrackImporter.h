#pragma once

#include <filesystem>

#include "track/import/TrackImport.h"

namespace openchordix::track::imports
{
    class ITrackImporter
    {
    public:
        virtual ~ITrackImporter() = default;

        virtual ImportFormat format() const = 0;
        virtual ImportResult<ImportedSong> importFile(const std::filesystem::path &path,
                                                      const ImportOptions &options = {}) const = 0;
    };
}
