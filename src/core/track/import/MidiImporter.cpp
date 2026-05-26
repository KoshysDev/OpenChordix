#include "track/import/MidiImporter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openchordix::track::imports
{
    namespace
    {
        class ByteReader
        {
        public:
            explicit ByteReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

            bool readByte(std::uint8_t &value)
            {
                if (position_ >= bytes_.size())
                {
                    return false;
                }
                value = bytes_[position_++];
                return true;
            }

            bool readBigEndian16(std::uint16_t &value)
            {
                std::uint8_t high = 0;
                std::uint8_t low = 0;
                if (!readByte(high) || !readByte(low))
                {
                    return false;
                }
                value = static_cast<std::uint16_t>((static_cast<std::uint16_t>(high) << 8U) | low);
                return true;
            }

            bool readBigEndian32(std::uint32_t &value)
            {
                std::array<std::uint8_t, 4> bytes{};
                for (std::uint8_t &byte : bytes)
                {
                    if (!readByte(byte))
                    {
                        return false;
                    }
                }
                value = (static_cast<std::uint32_t>(bytes[0]) << 24U) |
                        (static_cast<std::uint32_t>(bytes[1]) << 16U) |
                        (static_cast<std::uint32_t>(bytes[2]) << 8U) |
                        static_cast<std::uint32_t>(bytes[3]);
                return true;
            }

            bool readVariableLength(std::uint32_t &value)
            {
                value = 0;
                for (int i = 0; i < 4; ++i)
                {
                    std::uint8_t byte = 0;
                    if (!readByte(byte))
                    {
                        return false;
                    }
                    value = (value << 7U) | static_cast<std::uint32_t>(byte & 0x7FU);
                    if ((byte & 0x80U) == 0)
                    {
                        return true;
                    }
                }
                return false;
            }

            bool readSpan(std::size_t length, std::span<const std::uint8_t> &value)
            {
                if (length > remaining())
                {
                    return false;
                }
                value = bytes_.subspan(position_, length);
                position_ += length;
                return true;
            }

            bool skip(std::size_t length)
            {
                std::span<const std::uint8_t> unused;
                return readSpan(length, unused);
            }

            std::size_t remaining() const { return bytes_.size() - position_; }

        private:
            std::span<const std::uint8_t> bytes_;
            std::size_t position_ = 0;
        };

        bool readChunkType(ByteReader &reader, std::string_view type)
        {
            std::span<const std::uint8_t> chunkType;
            if (!reader.readSpan(type.size(), chunkType))
            {
                return false;
            }
            return std::equal(chunkType.begin(), chunkType.end(), type.begin(),
                              [](std::uint8_t left, char right)
                              { return left == static_cast<std::uint8_t>(right); });
        }

        struct ActiveNote
        {
            int tick = 0;
            int velocity = 0;
        };

        struct ParsedTrack
        {
            std::string name;
            std::vector<ImportedNote> notes;
            std::optional<int> program;
            int lastTick = 0;
        };

        ImportError malformed(std::string message)
        {
            return {ImportErrorCode::MalformedData, std::move(message)};
        }

        ImportResult<ParsedTrack> parseTrack(std::span<const std::uint8_t> bytes,
                                             ImportedSong &song,
                                             int trackIndex)
        {
            ByteReader reader(bytes);
            ParsedTrack track;
            std::map<int, std::vector<ActiveNote>> activeNotes;
            int absoluteTick = 0;
            std::uint8_t runningStatus = 0;

            while (reader.remaining() > 0)
            {
                std::uint32_t delta = 0;
                if (!reader.readVariableLength(delta))
                {
                    return ImportResult<ParsedTrack>::failure(malformed("MIDI track has an invalid delta time."));
                }
                if (delta > static_cast<std::uint32_t>(std::numeric_limits<int>::max() - absoluteTick))
                {
                    return ImportResult<ParsedTrack>::failure(malformed("MIDI track time exceeds supported range."));
                }
                absoluteTick += static_cast<int>(delta);
                track.lastTick = std::max(track.lastTick, absoluteTick);

                std::uint8_t eventByte = 0;
                if (!reader.readByte(eventByte))
                {
                    return ImportResult<ParsedTrack>::failure(malformed("MIDI track ends inside an event."));
                }

                std::uint8_t status = eventByte;
                bool hasFirstDataByte = false;
                std::uint8_t firstDataByte = 0;
                if (eventByte < 0x80U)
                {
                    if (runningStatus == 0)
                    {
                        return ImportResult<ParsedTrack>::failure(malformed("MIDI running status has no prior channel event."));
                    }
                    status = runningStatus;
                    hasFirstDataByte = true;
                    firstDataByte = eventByte;
                }
                else if (eventByte < 0xF0U)
                {
                    runningStatus = eventByte;
                }
                else
                {
                    runningStatus = 0;
                }

                if (status == 0xFFU)
                {
                    std::uint8_t metaType = 0;
                    std::uint32_t length = 0;
                    std::span<const std::uint8_t> data;
                    if (!reader.readByte(metaType) || !reader.readVariableLength(length) ||
                        !reader.readSpan(length, data))
                    {
                        return ImportResult<ParsedTrack>::failure(malformed("MIDI meta event is truncated."));
                    }

                    if (metaType == 0x03U)
                    {
                        track.name.assign(reinterpret_cast<const char *>(data.data()), data.size());
                    }
                    else if (metaType == 0x51U && data.size() == 3)
                    {
                        const std::uint32_t microsecondsPerQuarter =
                            (static_cast<std::uint32_t>(data[0]) << 16U) |
                            (static_cast<std::uint32_t>(data[1]) << 8U) |
                            static_cast<std::uint32_t>(data[2]);
                        if (microsecondsPerQuarter > 0)
                        {
                            song.tempos.push_back({
                                absoluteTick,
                                60000000.0 / static_cast<double>(microsecondsPerQuarter),
                            });
                        }
                    }
                    else if (metaType == 0x2FU)
                    {
                        break;
                    }
                    continue;
                }

                if (status == 0xF0U || status == 0xF7U)
                {
                    std::uint32_t length = 0;
                    if (!reader.readVariableLength(length) || !reader.skip(length))
                    {
                        return ImportResult<ParsedTrack>::failure(malformed("MIDI system-exclusive event is truncated."));
                    }
                    continue;
                }

                if (status >= 0xF0U)
                {
                    return ImportResult<ParsedTrack>::failure(malformed("MIDI contains an unsupported system event."));
                }

                const std::uint8_t command = status & 0xF0U;
                const int channel = status & 0x0FU;
                const bool oneDataByte = command == 0xC0U || command == 0xD0U;
                std::uint8_t data1 = 0;
                std::uint8_t data2 = 0;
                if (hasFirstDataByte)
                {
                    data1 = firstDataByte;
                }
                else if (!reader.readByte(data1))
                {
                    return ImportResult<ParsedTrack>::failure(malformed("MIDI channel event is truncated."));
                }
                if (!oneDataByte && !reader.readByte(data2))
                {
                    return ImportResult<ParsedTrack>::failure(malformed("MIDI channel event is truncated."));
                }

                if (command == 0xC0U && !track.program.has_value())
                {
                    track.program = static_cast<int>(data1);
                    continue;
                }

                if (command != 0x80U && command != 0x90U)
                {
                    continue;
                }

                const int key = channel * 128 + static_cast<int>(data1);
                const bool startsNote = command == 0x90U && data2 > 0;
                if (startsNote)
                {
                    activeNotes[key].push_back({absoluteTick, static_cast<int>(data2)});
                    continue;
                }

                auto active = activeNotes.find(key);
                if (active == activeNotes.end() || active->second.empty())
                {
                    continue;
                }
                const ActiveNote start = active->second.front();
                active->second.erase(active->second.begin());
                track.notes.push_back({
                    start.tick,
                    std::max(1, absoluteTick - start.tick),
                    static_cast<int>(data1),
                    start.velocity,
                    std::nullopt,
                    std::nullopt,
                });
            }

            if (track.name.empty())
            {
                track.name = "MIDI Track " + std::to_string(trackIndex + 1);
            }
            return ImportResult<ParsedTrack>::success(std::move(track));
        }

        double calculateDurationSeconds(const ImportedSong &song)
        {
            if (song.ticksPerBeat <= 0 || song.durationTicks <= 0)
            {
                return 0.0;
            }

            std::vector<TempoEvent> tempos = song.tempos;
            std::sort(tempos.begin(), tempos.end(),
                      [](const TempoEvent &left, const TempoEvent &right)
                      { return left.tick < right.tick; });
            double currentBpm = 120.0;
            int previousTick = 0;
            double seconds = 0.0;
            for (const TempoEvent &tempo : tempos)
            {
                const int tick = std::clamp(tempo.tick, previousTick, song.durationTicks);
                seconds += static_cast<double>(tick - previousTick) /
                           static_cast<double>(song.ticksPerBeat) * 60.0 / currentBpm;
                previousTick = tick;
                currentBpm = tempo.beatsPerMinute > 0.0 ? tempo.beatsPerMinute : currentBpm;
            }
            seconds += static_cast<double>(song.durationTicks - previousTick) /
                       static_cast<double>(song.ticksPerBeat) * 60.0 / currentBpm;
            return seconds;
        }
    }

    ImportResult<ImportedSong> MidiImporter::importFile(const std::filesystem::path &path,
                                                         const ImportOptions &options) const
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream.good())
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::FileReadFailed,
                "Failed to open MIDI file '" + path.string() + "'.",
            });
        }

        const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)),
                                              std::istreambuf_iterator<char>());
        return importBytes(bytes, path, options);
    }

    ImportResult<ImportedSong> MidiImporter::importBytes(std::span<const std::uint8_t> bytes,
                                                          std::filesystem::path sourcePath,
                                                          const ImportOptions &options) const
    {
        ByteReader reader(bytes);
        if (!readChunkType(reader, "MThd"))
        {
            return ImportResult<ImportedSong>::failure(malformed("MIDI header chunk is missing."));
        }

        std::uint32_t headerLength = 0;
        std::uint16_t midiFormat = 0;
        std::uint16_t trackCount = 0;
        std::uint16_t timeDivision = 0;
        if (!reader.readBigEndian32(headerLength) || headerLength < 6 ||
            !reader.readBigEndian16(midiFormat) || !reader.readBigEndian16(trackCount) ||
            !reader.readBigEndian16(timeDivision) || !reader.skip(headerLength - 6))
        {
            return ImportResult<ImportedSong>::failure(malformed("MIDI header is truncated or invalid."));
        }
        if (midiFormat > 1 || trackCount == 0)
        {
            return ImportResult<ImportedSong>::failure(malformed("Only standard MIDI formats 0 and 1 are supported."));
        }
        if ((timeDivision & 0x8000U) != 0 || timeDivision == 0)
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::UnsupportedFeature,
                "SMPTE MIDI time divisions are not supported for chart import.",
            });
        }

        ImportedSong song;
        song.format = ImportFormat::Midi;
        song.sourcePath = std::move(sourcePath);
        song.detectedFormat = "MIDI";
        song.title = song.sourcePath.stem().string();
        song.ticksPerBeat = static_cast<int>(timeDivision);

        for (int index = 0; index < static_cast<int>(trackCount); ++index)
        {
            std::uint32_t length = 0;
            std::span<const std::uint8_t> trackBytes;
            if (!readChunkType(reader, "MTrk") || !reader.readBigEndian32(length) ||
                !reader.readSpan(length, trackBytes))
            {
                return ImportResult<ImportedSong>::failure(malformed("MIDI track chunk is missing or truncated."));
            }

            auto parsedResult = parseTrack(trackBytes, song, index);
            if (!parsedResult)
            {
                return ImportResult<ImportedSong>::failure(parsedResult.error());
            }

            ParsedTrack parsed = std::move(parsedResult.value());
            song.durationTicks = std::max(song.durationTicks, parsed.lastTick);
            if (parsed.notes.empty() && !options.includeEmptyParts)
            {
                continue;
            }

            ImportedPart part;
            part.name = std::move(parsed.name);
            part.notes = std::move(parsed.notes);
            part.midiProgram = parsed.program;
            part.suggestedStringCount = openchordix::track::isBassLikePartName(part.name) ? 4 : openchordix::track::kDefaultTrackStrings;
            part.suggestedTuning = openchordix::track::defaultTuningForPart(part.name, part.suggestedStringCount);
            song.parts.push_back(std::move(part));
        }

        if (song.parts.empty())
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::NoImportableParts,
                "MIDI file contains no completed note events to import.",
            });
        }

        std::sort(song.tempos.begin(), song.tempos.end(),
                  [](const TempoEvent &left, const TempoEvent &right)
                  { return left.tick < right.tick; });
        song.durationSeconds = calculateDurationSeconds(song);
        return ImportResult<ImportedSong>::success(std::move(song));
    }
}
