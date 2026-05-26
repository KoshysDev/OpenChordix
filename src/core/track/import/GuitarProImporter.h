#pragma once

#include <cstdint>
#include <span>

#include "track/import/ITrackImporter.h"

namespace openchordix::track::imports
{
    class GuitarProImporter final : public ITrackImporter
    {
    public:
        ImportFormat format() const override { return ImportFormat::GuitarPro; }
        ImportResult<ImportedSong> importFile(const std::filesystem::path &path,
                                              const ImportOptions &options = {}) const override;
        ImportResult<ImportedSong> importBytes(std::span<const std::uint8_t> bytes,
                                               const std::filesystem::path &sourcePath = {},
                                               const ImportOptions &options = {}) const;
    };
}
