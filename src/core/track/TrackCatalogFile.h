#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "track/TrackCatalog.h"

class TrackCatalogFile final : public TrackCatalog
{
public:
    explicit TrackCatalogFile(std::filesystem::path storagePath = {});

    const std::vector<TrackInfo> &tracks() const override;
    std::vector<int> filter(const std::string &query) const override;
    bool reload() override;
    bool addTrack(const TrackInfo &track) override;
    bool updateTrack(std::string_view id, const TrackInfo &track) override;
    bool removeTrack(std::string_view id) override;
    const std::filesystem::path &storagePath() const override { return storagePath_; }

private:
    static std::string lowerCopy(const std::string &value);
    static std::string trimCopy(std::string_view value);
    static std::string slugify(std::string_view value);
    static std::string makeSearchBlob(const TrackInfo &track);
    static std::vector<TrackPart> normalizeParts(const std::vector<TrackPart> &parts);
    static std::filesystem::path resolveDefaultStoragePath();
    static std::filesystem::path resolveLegacyStoragePath();
    static std::filesystem::path resolveAbsoluteSongDirectory(const TrackInfo &track);
    static std::filesystem::path resolveAbsoluteChartPath(const TrackInfo &track);
    static bool mergeMetadataFromChart(const std::filesystem::path &chartPath, TrackInfo &track);

    std::string makeUniqueId(const TrackInfo &track) const;
    void applyTrackDefaults(TrackInfo &track) const;
    bool ensureSongChartAssets(const TrackInfo &track) const;
    bool isTrackValid(const TrackInfo &track) const;
    bool persist() const;
    void rebuildSearchIndex();

    std::filesystem::path storagePath_;
    std::vector<TrackInfo> tracks_;
    std::vector<std::string> searchIndex_;
};
