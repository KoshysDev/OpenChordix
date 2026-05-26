#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "track/import/ITrackImporter.h"

namespace openchordix::track::imports
{
    class ImporterRegistry
    {
    public:
        ImporterRegistry();

        static std::optional<ImportFormat> formatForPath(const std::filesystem::path &path);
        ImportResult<ImportedSong> importFile(const std::filesystem::path &path,
                                              const ImportOptions &options = {}) const;

    private:
        std::vector<std::unique_ptr<ITrackImporter>> importers_;
    };
}
