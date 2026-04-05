#include "track/TrackCatalogFile.h"

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <fstream>
#include <string_view>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include "AppPaths.h"
#include "track/TrackAudioDuration.h"

namespace
{
    using json = nlohmann::json;
    constexpr const char *kSongsDirectoryName = "Songs";
    constexpr const char *kDefaultChartFileName = "chart.ocx";

    json toJson(const TrackInfo &track)
    {
        json parts = json::array();
        for (const TrackPart &part : track.parts)
        {
            parts.push_back(part.name);
        }

        return json{
            {"id", track.id},
            {"title", track.title},
            {"artist", track.artist},
            {"source", track.source},
            {"mapper", track.mapper},
            {"bpm", track.bpm},
            {"length", track.length},
            {"parts", parts},
            {"directory", track.directory},
            {"chart_file", track.chartFile}};
    }

    TrackInfo fromJson(const json &value)
    {
        TrackInfo track;
        track.id = value.value("id", "");
        track.title = value.value("title", "");
        track.artist = value.value("artist", "");
        track.source = value.value("source", "");
        track.mapper = value.value("mapper", "");
        track.bpm = value.value("bpm", 0);
        track.length = value.value("length", "");
        track.directory = value.value("directory", "");
        track.audioFile = value.value("audio_file", "");
        track.chartFile = value.value("chart_file", "");

        if (const auto partsIt = value.find("parts"); partsIt != value.end() && partsIt->is_array())
        {
            for (const auto &partValue : *partsIt)
            {
                if (!partValue.is_string())
                {
                    continue;
                }
                track.parts.push_back(TrackPart{partValue.get<std::string>()});
            }
        }

        return track;
    }
}

TrackCatalogFile::TrackCatalogFile(std::filesystem::path storagePath)
{
    if (!storagePath.empty())
    {
        storagePath_ = std::move(storagePath);
    }
    else
    {
        storagePath_ = resolveDefaultStoragePath();
        const std::filesystem::path legacyPath = resolveLegacyStoragePath();
        if (!std::filesystem::exists(storagePath_) && std::filesystem::exists(legacyPath))
        {
            std::error_code ec;
            std::filesystem::rename(legacyPath, storagePath_, ec);
            if (ec)
            {
                storagePath_ = legacyPath;
            }
        }
    }
    reload();
}

const std::vector<TrackInfo> &TrackCatalogFile::tracks() const
{
    return tracks_;
}

std::vector<int> TrackCatalogFile::filter(const std::string &query) const
{
    std::vector<int> matches;
    matches.reserve(tracks_.size());

    const std::string loweredQuery = lowerCopy(query);
    if (loweredQuery.empty())
    {
        for (size_t i = 0; i < tracks_.size(); ++i)
        {
            matches.push_back(static_cast<int>(i));
        }
        return matches;
    }

    for (size_t i = 0; i < searchIndex_.size(); ++i)
    {
        if (searchIndex_[i].find(loweredQuery) != std::string::npos)
        {
            matches.push_back(static_cast<int>(i));
        }
    }
    return matches;
}

bool TrackCatalogFile::reload()
{
    if (!std::filesystem::exists(storagePath_))
    {
        tracks_.clear();
        searchIndex_.clear();
        return true;
    }

    std::ifstream in(storagePath_);
    if (!in.good())
    {
        return false;
    }

    json root;
    try
    {
        in >> root;
    }
    catch (const json::exception &)
    {
        return false;
    }

    const auto songsIt = root.find("songs");
    if (songsIt == root.end() || !songsIt->is_array())
    {
        return false;
    }

    std::vector<TrackInfo> loaded;
    loaded.reserve(songsIt->size());
    for (const auto &entry : *songsIt)
    {
        if (!entry.is_object())
        {
            continue;
        }

        TrackInfo track = fromJson(entry);
        track.title = trimCopy(track.title);
        track.artist = trimCopy(track.artist);
        track.source = trimCopy(track.source);
        track.mapper = trimCopy(track.mapper);
        track.length = trimCopy(track.length);
        track.directory = trimCopy(track.directory);
        track.audioFile = trimCopy(track.audioFile);
        track.chartFile = trimCopy(track.chartFile);

        if (track.id.empty())
        {
            std::string baseId = slugify(track.artist + "-" + track.title);
            std::string candidateId = baseId;
            int suffix = 2;
            while (std::any_of(
                loaded.begin(),
                loaded.end(),
                [&](const TrackInfo &existing)
                {
                    return existing.id == candidateId;
                }))
            {
                candidateId = baseId + "-" + std::to_string(suffix);
                ++suffix;
            }
            track.id = std::move(candidateId);
        }

        applyTrackDefaults(track);
        mergeMetadataFromChart(resolveAbsoluteChartPath(track), track);
        track.title = trimCopy(track.title);
        track.artist = trimCopy(track.artist);
        track.source = trimCopy(track.source);
        track.mapper = trimCopy(track.mapper);
        track.length = trimCopy(track.length);
        track.audioFile = trimCopy(track.audioFile);
        track.parts = normalizeParts(track.parts);

        if (!isTrackValid(track))
        {
            continue;
        }

        const bool duplicatedId = std::any_of(
            loaded.begin(),
            loaded.end(),
            [&](const TrackInfo &existing)
            {
                return existing.id == track.id;
            });
        if (duplicatedId)
        {
            continue;
        }

        loaded.push_back(std::move(track));
    }

    tracks_ = std::move(loaded);
    rebuildSearchIndex();
    return true;
}

bool TrackCatalogFile::addTrack(const TrackInfo &track)
{
    TrackInfo normalized = track;
    normalized.title = trimCopy(normalized.title);
    normalized.artist = trimCopy(normalized.artist);
    normalized.source = trimCopy(normalized.source);
    normalized.mapper = trimCopy(normalized.mapper);
    normalized.length = trimCopy(normalized.length);
    normalized.directory = trimCopy(normalized.directory);
    normalized.audioFile = trimCopy(normalized.audioFile);
    normalized.chartFile = trimCopy(normalized.chartFile);
    normalized.parts = normalizeParts(normalized.parts);

    if (!isTrackValid(normalized))
    {
        return false;
    }

    normalized.id = makeUniqueId(normalized);
    if (!normalized.audioFile.empty())
    {
        std::filesystem::path resolvedAudioPath = normalized.audioFile;
        if (!normalized.directory.empty() && resolvedAudioPath.is_relative())
        {
            resolvedAudioPath = std::filesystem::path(normalized.directory) / resolvedAudioPath;
        }

        if (auto detectedDuration = openchordix::track::readAudioDuration(resolvedAudioPath))
        {
            normalized.length = *detectedDuration;
        }
    }

    if (normalized.source.empty())
    {
        normalized.source = "Custom";
    }
    applyTrackDefaults(normalized);
    if (!ensureSongChartAssets(normalized))
    {
        return false;
    }

    tracks_.push_back(normalized);
    searchIndex_.push_back(makeSearchBlob(normalized));
    if (!persist())
    {
        tracks_.pop_back();
        searchIndex_.pop_back();
        return false;
    }
    return true;
}

bool TrackCatalogFile::updateTrack(std::string_view id, const TrackInfo &track)
{
    const auto it = std::find_if(
        tracks_.begin(),
        tracks_.end(),
        [&](const TrackInfo &existing)
        {
            return existing.id == id;
        });
    if (it == tracks_.end())
    {
        return false;
    }

    TrackInfo normalized = track;
    const TrackInfo current = *it;
    normalized.id = std::string(id);
    normalized.title = trimCopy(normalized.title);
    normalized.artist = trimCopy(normalized.artist);
    normalized.source = trimCopy(normalized.source);
    normalized.mapper = trimCopy(normalized.mapper);
    normalized.length = trimCopy(normalized.length);
    normalized.directory = trimCopy(normalized.directory);
    normalized.audioFile = trimCopy(normalized.audioFile);
    normalized.chartFile = trimCopy(normalized.chartFile);
    normalized.parts = normalizeParts(normalized.parts);

    if (!isTrackValid(normalized))
    {
        return false;
    }

    if (normalized.directory.empty())
    {
        normalized.directory = current.directory;
    }
    if (normalized.audioFile.empty())
    {
        normalized.audioFile = current.audioFile;
    }
    if (normalized.chartFile.empty())
    {
        normalized.chartFile = current.chartFile;
    }
    if (normalized.length.empty())
    {
        normalized.length = current.length;
    }

    if (!normalized.audioFile.empty())
    {
        std::filesystem::path resolvedAudioPath = normalized.audioFile;
        if (!normalized.directory.empty() && resolvedAudioPath.is_relative())
        {
            resolvedAudioPath = std::filesystem::path(normalized.directory) / resolvedAudioPath;
        }

        if (auto detectedDuration = openchordix::track::readAudioDuration(resolvedAudioPath))
        {
            normalized.length = *detectedDuration;
        }
    }

    if (normalized.source.empty())
    {
        normalized.source = "Custom";
    }
    applyTrackDefaults(normalized);
    if (!ensureSongChartAssets(normalized))
    {
        return false;
    }

    const size_t index = static_cast<size_t>(std::distance(tracks_.begin(), it));
    TrackInfo previous = tracks_[index];
    const std::string previousSearch = searchIndex_[index];

    tracks_[index] = normalized;
    searchIndex_[index] = makeSearchBlob(normalized);
    if (!persist())
    {
        tracks_[index] = std::move(previous);
        searchIndex_[index] = previousSearch;
        return false;
    }

    return true;
}

bool TrackCatalogFile::removeTrack(std::string_view id)
{
    const auto it = std::find_if(
        tracks_.begin(),
        tracks_.end(),
        [&](const TrackInfo &track)
        {
            return track.id == id;
        });

    if (it == tracks_.end())
    {
        return false;
    }

    const size_t index = static_cast<size_t>(std::distance(tracks_.begin(), it));
    TrackInfo removed = *it;
    const std::string removedSearch = searchIndex_[index];

    tracks_.erase(it);
    searchIndex_.erase(searchIndex_.begin() + static_cast<std::ptrdiff_t>(index));

    if (!persist())
    {
        tracks_.insert(tracks_.begin() + static_cast<std::ptrdiff_t>(index), removed);
        searchIndex_.insert(searchIndex_.begin() + static_cast<std::ptrdiff_t>(index), removedSearch);
        return false;
    }

    return true;
}

std::string TrackCatalogFile::lowerCopy(const std::string &value)
{
    std::string out = value;
    std::transform(
        out.begin(),
        out.end(),
        out.begin(),
        [](unsigned char ch)
        {
            return static_cast<char>(std::tolower(ch));
        });
    return out;
}

std::string TrackCatalogFile::trimCopy(std::string_view value)
{
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
    {
        ++first;
    }

    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
    {
        --last;
    }

    return std::string(value.substr(first, last - first));
}

std::string TrackCatalogFile::slugify(std::string_view value)
{
    std::string slug;
    slug.reserve(value.size());
    bool pendingDash = false;

    for (char raw : value)
    {
        const unsigned char ch = static_cast<unsigned char>(raw);
        if (std::isalnum(ch))
        {
            if (pendingDash && !slug.empty())
            {
                slug.push_back('-');
            }
            slug.push_back(static_cast<char>(std::tolower(ch)));
            pendingDash = false;
        }
        else if (!slug.empty())
        {
            pendingDash = true;
        }
    }

    if (slug.empty())
    {
        return "song";
    }
    return slug;
}

std::string TrackCatalogFile::makeSearchBlob(const TrackInfo &track)
{
    return lowerCopy(track.title + " " + track.artist + " " + track.source + " " + track.mapper);
}

std::vector<TrackPart> TrackCatalogFile::normalizeParts(const std::vector<TrackPart> &parts)
{
    std::vector<TrackPart> normalized;
    normalized.reserve(parts.size());
    for (const TrackPart &part : parts)
    {
        const std::string name = trimCopy(part.name);
        if (name.empty())
        {
            continue;
        }
        normalized.push_back(TrackPart{name});
    }
    return normalized;
}

std::filesystem::path TrackCatalogFile::resolveDefaultStoragePath()
{
    return openchordix::core::executableDirectory() / "songs.db";
}

std::filesystem::path TrackCatalogFile::resolveLegacyStoragePath()
{
    return openchordix::core::executableDirectory() / "songs_db.json";
}

std::filesystem::path TrackCatalogFile::resolveAbsoluteSongDirectory(const TrackInfo &track)
{
    std::filesystem::path songDir(track.directory);
    if (songDir.empty())
    {
        return openchordix::core::executableDirectory() / kSongsDirectoryName / track.id;
    }
    if (songDir.is_relative())
    {
        return openchordix::core::executableDirectory() / songDir;
    }
    return songDir;
}

std::filesystem::path TrackCatalogFile::resolveAbsoluteChartPath(const TrackInfo &track)
{
    std::filesystem::path chartPath(track.chartFile);
    if (chartPath.empty())
    {
        chartPath = kDefaultChartFileName;
    }
    if (chartPath.is_relative())
    {
        return resolveAbsoluteSongDirectory(track) / chartPath;
    }
    return chartPath;
}

bool TrackCatalogFile::mergeMetadataFromChart(const std::filesystem::path &chartPath, TrackInfo &track)
{
    if (!std::filesystem::exists(chartPath))
    {
        return false;
    }

    std::ifstream in(chartPath);
    if (!in.good())
    {
        return false;
    }

    json root;
    try
    {
        in >> root;
    }
    catch (const json::exception &)
    {
        return false;
    }

    if (!root.is_object())
    {
        return false;
    }

    if (const auto idIt = root.find("song_id"); idIt != root.end() && idIt->is_string())
    {
        track.id = trimCopy(idIt->get<std::string>());
    }
    if (const auto titleIt = root.find("title"); titleIt != root.end() && titleIt->is_string())
    {
        track.title = trimCopy(titleIt->get<std::string>());
    }
    if (const auto artistIt = root.find("artist"); artistIt != root.end() && artistIt->is_string())
    {
        track.artist = trimCopy(artistIt->get<std::string>());
    }
    if (const auto sourceIt = root.find("source"); sourceIt != root.end() && sourceIt->is_string())
    {
        track.source = trimCopy(sourceIt->get<std::string>());
    }
    if (const auto mapperIt = root.find("mapper"); mapperIt != root.end() && mapperIt->is_string())
    {
        track.mapper = trimCopy(mapperIt->get<std::string>());
    }
    if (const auto lengthIt = root.find("length"); lengthIt != root.end() && lengthIt->is_string())
    {
        track.length = trimCopy(lengthIt->get<std::string>());
    }
    if (const auto audioIt = root.find("audio_file"); audioIt != root.end() && audioIt->is_string())
    {
        track.audioFile = trimCopy(audioIt->get<std::string>());
    }
    if (const auto bpmIt = root.find("bpm"); bpmIt != root.end() && bpmIt->is_number_integer())
    {
        track.bpm = bpmIt->get<int>();
    }

    if (const auto partsIt = root.find("parts"); partsIt != root.end() && partsIt->is_array())
    {
        track.parts.clear();
        for (const auto &partValue : *partsIt)
        {
            if (!partValue.is_string())
            {
                continue;
            }
            track.parts.push_back(TrackPart{trimCopy(partValue.get<std::string>())});
        }
    }

    return true;
}

std::string TrackCatalogFile::makeUniqueId(const TrackInfo &track) const
{
    std::string base = trimCopy(track.id);
    if (base.empty())
    {
        base = slugify(track.artist + "-" + track.title);
    }

    std::string candidate = base;
    int suffix = 2;
    while (std::any_of(
        tracks_.begin(),
        tracks_.end(),
        [&](const TrackInfo &existing)
        {
            return existing.id == candidate;
        }))
    {
        candidate = base + "-" + std::to_string(suffix);
        ++suffix;
    }

    return candidate;
}

void TrackCatalogFile::applyTrackDefaults(TrackInfo &track) const
{
    if (track.mapper.empty())
    {
        track.mapper = "Unknown";
    }
    if (track.length.empty())
    {
        track.length = "--:--";
    }
    if (track.directory.empty())
    {
        track.directory = (std::filesystem::path(kSongsDirectoryName) / track.id).generic_string();
    }
    if (track.chartFile.empty())
    {
        track.chartFile = kDefaultChartFileName;
    }
}

bool TrackCatalogFile::ensureSongChartAssets(const TrackInfo &track) const
{
    std::error_code ec;
    const std::filesystem::path songDir = resolveAbsoluteSongDirectory(track);
    std::filesystem::create_directories(songDir, ec);
    if (ec)
    {
        return false;
    }

    const std::filesystem::path chartPath = resolveAbsoluteChartPath(track);
    json chartRoot;
    if (std::filesystem::exists(chartPath))
    {
        std::ifstream in(chartPath);
        if (in.good())
        {
            try
            {
                in >> chartRoot;
            }
            catch (const json::exception &)
            {
                chartRoot = json::object();
            }
        }
    }

    if (!chartRoot.is_object())
    {
        chartRoot = json::object();
    }

    if (!chartRoot.contains("events") || !chartRoot["events"].is_array())
    {
        chartRoot["events"] = json::array();
    }
    if (!chartRoot.contains("notes") || !chartRoot["notes"].is_array())
    {
        chartRoot["notes"] = json::array();
    }

    int version = 1;
    if (const auto versionIt = chartRoot.find("version");
        versionIt != chartRoot.end() && versionIt->is_number_integer())
    {
        version = versionIt->get<int>();
    }
    chartRoot["version"] = version;
    chartRoot["song_id"] = track.id;
    chartRoot["title"] = track.title;
    chartRoot["artist"] = track.artist;
    chartRoot["source"] = track.source;
    chartRoot["mapper"] = track.mapper;
    chartRoot["bpm"] = track.bpm;
    chartRoot["length"] = track.length;
    chartRoot["audio_file"] = track.audioFile;
    chartRoot["parts"] = json::array();
    for (const TrackPart &part : track.parts)
    {
        chartRoot["parts"].push_back(part.name);
    }

    std::ofstream out(chartPath, std::ios::trunc);
    if (!out.good())
    {
        return false;
    }
    out << chartRoot.dump(2) << '\n';
    return out.good();
}

bool TrackCatalogFile::isTrackValid(const TrackInfo &track) const
{
    if (track.title.empty() || track.artist.empty())
    {
        return false;
    }

    if (track.bpm <= 0)
    {
        return false;
    }

    if (track.parts.empty())
    {
        return false;
    }

    return true;
}

bool TrackCatalogFile::persist() const
{
    std::error_code ec;
    const std::filesystem::path parentDir = storagePath_.parent_path();
    if (!parentDir.empty())
    {
        std::filesystem::create_directories(parentDir, ec);
        if (ec)
        {
            return false;
        }
    }

    json root;
    root["version"] = 1;
    root["songs"] = json::array();
    for (const TrackInfo &track : tracks_)
    {
        root["songs"].push_back(toJson(track));
    }

    const std::filesystem::path tempPath = storagePath_.string() + ".tmp";
    {
        std::ofstream out(tempPath, std::ios::trunc);
        if (!out.good())
        {
            return false;
        }
        out << root.dump(2) << '\n';
        out.flush();
        if (!out.good())
        {
            return false;
        }
    }

    std::filesystem::rename(tempPath, storagePath_, ec);
    if (!ec)
    {
        return true;
    }

    std::error_code removeEc;
    std::filesystem::remove(storagePath_, removeEc);
    ec.clear();
    std::filesystem::rename(tempPath, storagePath_, ec);
    if (ec)
    {
        std::filesystem::remove(tempPath, removeEc);
        return false;
    }
    return true;
}

void TrackCatalogFile::rebuildSearchIndex()
{
    searchIndex_.clear();
    searchIndex_.reserve(tracks_.size());
    for (const TrackInfo &track : tracks_)
    {
        searchIndex_.push_back(makeSearchBlob(track));
    }
}
