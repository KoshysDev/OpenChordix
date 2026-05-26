#include "track/import/ImporterRegistry.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "track/import/GuitarProImporter.h"
#include "track/import/MidiImporter.h"

namespace openchordix::track::imports
{
    namespace
    {
        std::string lowerExtension(const std::filesystem::path &path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char value)
                           { return static_cast<char>(std::tolower(value)); });
            return extension;
        }
    }

    ImporterRegistry::ImporterRegistry()
    {
        importers_.push_back(std::make_unique<MidiImporter>());
        importers_.push_back(std::make_unique<GuitarProImporter>());
    }

    std::optional<ImportFormat> ImporterRegistry::formatForPath(const std::filesystem::path &path)
    {
        const std::string extension = lowerExtension(path);
        if (extension == ".mid" || extension == ".midi")
        {
            return ImportFormat::Midi;
        }
        if (extension == ".gp3" || extension == ".gp4" || extension == ".gp5" ||
            extension == ".gpx" || extension == ".gp")
        {
            return ImportFormat::GuitarPro;
        }
        return std::nullopt;
    }

    ImportResult<ImportedSong> ImporterRegistry::importFile(const std::filesystem::path &path,
                                                             const ImportOptions &options) const
    {
        const auto detectedFormat = formatForPath(path);
        if (!detectedFormat.has_value())
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::UnsupportedFormat,
                "No chart importer is available for extension '" + path.extension().string() + "'.",
            });
        }

        for (const auto &importer : importers_)
        {
            if (importer->format() == *detectedFormat)
            {
                return importer->importFile(path, options);
            }
        }

        return ImportResult<ImportedSong>::failure({
            ImportErrorCode::UnsupportedFormat,
            "The detected chart format has no registered importer.",
        });
    }
}
