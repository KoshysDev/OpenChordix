#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

#include "track/import/TrackImport.h"

namespace openchordix::track::imports
{
    class GuitarPro7Importer
    {
    public:
        ImportResult<ImportedSong> importBytes(std::span<const std::uint8_t> bytes,
                                               const std::filesystem::path &sourcePath = {},
                                               const ImportOptions &options = {}) const;
    };
}
