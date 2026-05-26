#pragma once

#include <cstdint>
#include <span>

#include "track/import/ITrackImporter.h"

namespace openchordix::track::imports
{
    class MidiImporter final : public ITrackImporter
    {
    public:
        ImportFormat format() const override { return ImportFormat::Midi; }
        ImportResult<ImportedSong> importFile(const std::filesystem::path &path,
                                              const ImportOptions &options = {}) const override;
        ImportResult<ImportedSong> importBytes(std::span<const std::uint8_t> bytes,
                                               std::filesystem::path sourcePath = {},
                                               const ImportOptions &options = {}) const;
    };
}
