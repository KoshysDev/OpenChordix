#include "track/import/GuitarPro7Importer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <miniz.h>
#include <pugixml.hpp>

#include "track/TempoMap.h"
#include "track/TrackTiming.h"
#include "track/TuningLibrary.h"

namespace openchordix::track::imports
{
    namespace
    {
        constexpr int kGpifTicksPerBeat = 960;
        constexpr std::size_t kMaximumScoreSize = 64U * 1024U * 1024U;
        constexpr std::int64_t kMaximumTimingDenominator = 1000000;

        struct Fraction
        {
            std::int64_t numerator = 0;
            std::int64_t denominator = 1;
        };

        struct Rhythm
        {
            Fraction duration;
            bool valid = false;
            bool rounded = false;
        };

        struct ParsedBeat
        {
            std::string rhythmId;
            std::vector<std::string> noteIds;
            ImportedTechnique technique;
        };

        struct ParsedVoice
        {
            std::vector<std::string> beatIds;
        };

        struct ParsedBar
        {
            std::vector<std::string> voiceIds;
        };

        struct ParsedTrack
        {
            std::string id;
            ImportedPart part;
        };

        struct ParsedMasterBar
        {
            ImportedMeasure measure;
            std::vector<std::string> barIds;
        };

        struct ZipCandidate
        {
            int rank = 0;
            mz_uint index = 0;
            std::string name;
            bool encrypted = false;
            std::uint64_t uncompressedSize = 0;
        };

        class ZipArchive
        {
        public:
            explicit ZipArchive(std::span<const std::uint8_t> bytes)
            {
                initialized_ = mz_zip_reader_init_mem(&archive_, bytes.data(), bytes.size(), 0) != 0;
            }

            ~ZipArchive()
            {
                if (initialized_)
                {
                    mz_zip_reader_end(&archive_);
                }
            }

            bool valid() const { return initialized_; }
            mz_zip_archive &get() { return archive_; }

        private:
            mz_zip_archive archive_{};
            bool initialized_ = false;
        };

        std::string lowercase(std::string text)
        {
            std::transform(text.begin(), text.end(), text.begin(),
                           [](unsigned char value)
                           { return static_cast<char>(std::tolower(value)); });
            return text;
        }

        std::vector<std::string> splitText(std::string_view value)
        {
            std::vector<std::string> fields;
            std::size_t start = 0;
            while (start < value.size())
            {
                while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
                {
                    ++start;
                }
                std::size_t end = start;
                while (end < value.size() && std::isspace(static_cast<unsigned char>(value[end])) == 0)
                {
                    ++end;
                }
                if (end > start)
                {
                    fields.emplace_back(value.substr(start, end - start));
                }
                start = end;
            }
            return fields;
        }

        std::optional<int> parseInt(std::string_view value)
        {
            if (value.empty())
            {
                return std::nullopt;
            }
            std::string text(value);
            char *end = nullptr;
            const long parsed = std::strtol(text.c_str(), &end, 10);
            if (end == text.c_str() || *end != '\0' ||
                parsed < std::numeric_limits<int>::min() ||
                parsed > std::numeric_limits<int>::max())
            {
                return std::nullopt;
            }
            return static_cast<int>(parsed);
        }

        std::optional<int> parseMajorVersion(std::string_view value)
        {
            const std::size_t separator = value.find('.');
            return parseInt(value.substr(0, separator));
        }

        std::optional<double> parseDouble(std::string_view value)
        {
            if (value.empty())
            {
                return std::nullopt;
            }
            std::string text(value);
            char *end = nullptr;
            const double parsed = std::strtod(text.c_str(), &end);
            return end == text.c_str() || *end != '\0' ? std::nullopt : std::optional<double>(parsed);
        }

        void reduce(Fraction &value)
        {
            if (value.denominator <= 0)
            {
                value = {};
                return;
            }
            const std::int64_t factor = std::gcd(value.numerator, value.denominator);
            if (factor > 1)
            {
                value.numerator /= factor;
                value.denominator /= factor;
            }
        }

        Fraction multiply(Fraction value, std::int64_t numerator, std::int64_t denominator)
        {
            value.numerator *= numerator;
            value.denominator *= denominator;
            reduce(value);
            return value;
        }

        std::optional<Fraction> add(Fraction left, Fraction right)
        {
            const std::int64_t divisor = std::gcd(left.denominator, right.denominator);
            const std::int64_t multiplier = right.denominator / divisor;
            if (left.denominator > kMaximumTimingDenominator / multiplier)
            {
                return std::nullopt;
            }
            const std::int64_t common = left.denominator * multiplier;
            left.numerator = left.numerator * (common / left.denominator) +
                             right.numerator * (common / right.denominator);
            left.denominator = common;
            reduce(left);
            return left.denominator > kMaximumTimingDenominator
                       ? std::nullopt
                       : std::optional<Fraction>(left);
        }

        int roundTicks(Fraction value)
        {
            const std::int64_t rounded = (value.numerator + value.denominator / 2) / value.denominator;
            return static_cast<int>(std::clamp<std::int64_t>(rounded, 0, std::numeric_limits<int>::max()));
        }

        void addWarning(ImportedPart &part, std::string_view warning)
        {
            if (part.status.find(warning) != std::string::npos)
            {
                return;
            }
            if (!part.status.empty())
            {
                part.status += "; ";
            }
            part.status += warning;
        }

        std::string childText(const pugi::xml_node &node, const char *name)
        {
            return node.child(name).text().as_string();
        }

        std::optional<std::string> propertyText(const pugi::xml_node &node, const char *property,
                                                const char *valueElement)
        {
            for (const pugi::xml_node candidate : node.child("Properties").children("Property"))
            {
                if (std::string_view(candidate.attribute("name").as_string()) == property)
                {
                    return childText(candidate, valueElement);
                }
            }
            return std::nullopt;
        }

        std::vector<ZipCandidate> scoreCandidates(mz_zip_archive &archive)
        {
            std::vector<ZipCandidate> candidates;
            const mz_uint count = mz_zip_reader_get_num_files(&archive);
            candidates.reserve(count);
            for (mz_uint index = 0; index < count; ++index)
            {
                mz_zip_archive_file_stat stat{};
                if (!mz_zip_reader_file_stat(&archive, index, &stat) || stat.m_is_directory != 0)
                {
                    continue;
                }
                const std::string name = stat.m_filename == nullptr ? "" : stat.m_filename;
                const std::string normalized = lowercase(name);
                int rank = -1;
                if (normalized == "content/score.gpif")
                {
                    rank = 0;
                }
                else if (normalized == "score.gpif")
                {
                    rank = 1;
                }
                else if (normalized.size() > 11 &&
                         normalized.compare(normalized.size() - 11, 11, "/score.gpif") == 0)
                {
                    rank = 2;
                }
                if (rank >= 0)
                {
                    candidates.push_back({
                        rank,
                        index,
                        name,
                        (stat.m_bit_flag & 1U) != 0,
                        static_cast<std::uint64_t>(stat.m_uncomp_size),
                    });
                }
            }
            std::sort(candidates.begin(), candidates.end(),
                      [](const ZipCandidate &left, const ZipCandidate &right)
                      {
                          if (left.rank != right.rank)
                          {
                              return left.rank < right.rank;
                          }
                          return lowercase(left.name) < lowercase(right.name);
                      });
            return candidates;
        }

        ImportResult<std::string> readScoreDocument(std::span<const std::uint8_t> bytes)
        {
            ZipArchive archive(bytes);
            if (!archive.valid())
            {
                return ImportResult<std::string>::failure({
                    ImportErrorCode::MalformedData,
                    "Failed to read ZIP archive while checking for a Guitar Pro score document.",
                    "ZIP archive",
                });
            }
            const std::vector<ZipCandidate> candidates = scoreCandidates(archive.get());
            if (candidates.empty())
            {
                return ImportResult<std::string>::failure({
                    ImportErrorCode::UnsupportedFormat,
                    "ZIP archive does not contain a readable score.gpif document.",
                    "ZIP archive",
                });
            }
            const ZipCandidate &candidate = candidates.front();
            if (candidate.encrypted)
            {
                return ImportResult<std::string>::failure({
                    ImportErrorCode::UnsupportedFeature,
                    "The Guitar Pro score document is encrypted; protected packages are not supported.",
                    "Guitar Pro 7+ (zip package)",
                });
            }
            if (candidate.uncompressedSize > kMaximumScoreSize)
            {
                return ImportResult<std::string>::failure({
                    ImportErrorCode::UnsupportedFeature,
                    "The Guitar Pro score document is too large to import safely.",
                    "Guitar Pro 7+ (zip package)",
                });
            }
            size_t size = 0;
            void *extracted = mz_zip_reader_extract_to_heap(&archive.get(), candidate.index, &size, 0);
            if (extracted == nullptr)
            {
                return ImportResult<std::string>::failure({
                    ImportErrorCode::MalformedData,
                    "Failed to extract the readable Guitar Pro score document from the ZIP archive.",
                    "ZIP package (score.gpif found)",
                });
            }
            std::string xml(static_cast<const char *>(extracted), size);
            mz_free(extracted);
            return ImportResult<std::string>::success(std::move(xml));
        }

        std::optional<Fraction> noteValueDuration(std::string_view noteValue)
        {
            static const std::unordered_map<std::string, int> ticks{
                {"Long", 15360}, {"DoubleWhole", 7680}, {"Whole", 3840}, {"Half", 1920},
                {"Quarter", 960}, {"Eighth", 480}, {"16th", 240}, {"32nd", 120},
                {"64th", 60}, {"128th", 30}, {"256th", 15},
            };
            const auto found = ticks.find(std::string(noteValue));
            return found == ticks.end() ? std::nullopt : std::optional<Fraction>(Fraction{found->second, 1});
        }

        ImportedTechnique parseNoteTechnique(const pugi::xml_node &note)
        {
            ImportedTechnique result;
            if (note.child("LetRing"))
            {
                result.letRing = true;
            }
            if (note.child("Trill"))
            {
                result.trill = true;
            }
            const int accent = parseInt(childText(note, "Accent")).value_or(0);
            result.staccato = (accent & 0x01) != 0;
            result.heavyAccent = (accent & 0x04) != 0;
            result.accent = (accent & 0x08) != 0;
            result.vibrato = note.child("Vibrato") != nullptr;
            result.noteType = note.child("Tie") ? "tie" : "normal";

            const auto harmonic = propertyText(note, "HarmonicType", "HType");
            if (harmonic.has_value() && lowercase(*harmonic) != "noharmonic")
            {
                result.harmonicType = lowercase(*harmonic);
            }
            result.palmMute = propertyText(note, "PalmMuted", "Enable").has_value();
            if (propertyText(note, "Muted", "Enable").has_value())
            {
                result.noteType = "dead";
            }
            result.bend = propertyText(note, "Bended", "Enable").has_value();
            result.hammerOn = propertyText(note, "HopoOrigin", "Enable").has_value();
            if (propertyText(note, "Tapped", "Enable").has_value() ||
                propertyText(note, "LeftHandTapped", "Enable").has_value())
            {
                result.pluckStyle = "tap";
            }
            const auto slide = propertyText(note, "Slide", "Flags");
            if (slide.has_value())
            {
                const int flags = parseInt(*slide).value_or(0);
                if ((flags & 2) != 0)
                {
                    result.slideType = "legato";
                }
                else if ((flags & 1) != 0)
                {
                    result.slideType = "shift";
                }
                else if ((flags & 4) != 0)
                {
                    result.slideType = "out-down";
                }
                else if ((flags & 8) != 0)
                {
                    result.slideType = "out-up";
                }
                else if ((flags & 16) != 0)
                {
                    result.slideType = "in-from-below";
                }
                else if ((flags & 32) != 0)
                {
                    result.slideType = "in-from-above";
                }
                else if ((flags & 64) != 0)
                {
                    result.slideType = "pick-down";
                }
                else if ((flags & 128) != 0)
                {
                    result.slideType = "pick-up";
                }
            }
            return result;
        }

        void mergeBeatTechnique(ImportedTechnique &note, const ImportedTechnique &beat)
        {
            note.vibrato = note.vibrato || beat.vibrato;
            note.tremoloPicking = note.tremoloPicking || beat.tremoloPicking;
            if (note.pluckStyle.empty())
            {
                note.pluckStyle = beat.pluckStyle;
            }
        }

        ImportedTechnique parseBeatTechnique(const pugi::xml_node &beat)
        {
            ImportedTechnique result;
            result.tremoloPicking = beat.child("Tremolo") != nullptr;
            for (const pugi::xml_node property : beat.child("Properties").children("Property"))
            {
                const std::string name = property.attribute("name").as_string();
                if (name == "Slapped" && property.child("Enable"))
                {
                    result.pluckStyle = "slap";
                }
                else if (name == "Popped" && property.child("Enable"))
                {
                    result.pluckStyle = "pop";
                }
                else if (name == "VibratoWTremBar" && property.child("Strength"))
                {
                    result.vibrato = true;
                }
            }
            return result;
        }

        class GpifAdapter
        {
        public:
            GpifAdapter(const pugi::xml_node &root, std::filesystem::path sourcePath, std::string format)
                : root_(root), sourcePath_(std::move(sourcePath)), detectedFormat_(std::move(format))
            {
            }

            ImportResult<ImportedSong> convert()
            {
                ImportedSong song;
                song.format = ImportFormat::GuitarPro;
                song.sourcePath = sourcePath_;
                song.detectedFormat = detectedFormat_;
                song.ticksPerBeat = kGpifTicksPerBeat;
                readMetadata(song);
                readMasterBars(song);
                if (song.measures.empty())
                {
                    return failed("GPIF document has no supported master bars.");
                }
                readRhythms();
                readNotes();
                readBeats();
                readVoices();
                readBars();
                adjustPickupMeasureTiming(song);
                readTracks();
                readTempo(song);
                materializeParts(song);
                if (song.parts.empty())
                {
                    return ImportResult<ImportedSong>::failure({
                        ImportErrorCode::NoImportableParts,
                        "GPIF document contains no recognized tracks.",
                        detectedFormat_,
                    });
                }
                const ImportedMeasure &last = song.measures.back();
                song.durationTicks = last.startTick + last.durationTicks;
                song.durationSeconds = tempoMap(song).tickToSeconds(song.durationTicks);
                return ImportResult<ImportedSong>::success(std::move(song));
            }

        private:
            ImportResult<ImportedSong> failed(std::string message) const
            {
                return ImportResult<ImportedSong>::failure({
                    ImportErrorCode::MalformedData, std::move(message), detectedFormat_});
            }

            void readMetadata(ImportedSong &song) const
            {
                const pugi::xml_node score = root_.child("Score");
                song.title = childText(score, "Title");
                song.subtitle = childText(score, "SubTitle");
                song.artist = childText(score, "Artist");
                song.album = childText(score, "Album");
            }

            void readMasterBars(ImportedSong &song)
            {
                int startTick = 0;
                int number = 1;
                const bool hasPickup = root_.child("MasterTrack").child("Anacrusis") != nullptr;
                for (const pugi::xml_node node : root_.child("MasterBars").children("MasterBar"))
                {
                    const std::string time = childText(node, "Time");
                    const std::size_t slash = time.find('/');
                    int numerator = 4;
                    int denominator = 4;
                    if (slash != std::string::npos)
                    {
                        numerator = parseInt(time.substr(0, slash)).value_or(4);
                        denominator = parseInt(time.substr(slash + 1)).value_or(4);
                    }
                    const auto durationTicks =
                        openchordix::track::measureLengthTicks(kGpifTicksPerBeat, numerator, denominator);
                    if (!durationTicks.has_value() || numerator > 1000 || denominator > 1024)
                    {
                        timingWarnings_.push_back("Skipped master bar with unsupported time signature '" + time + "'");
                        continue;
                    }
                    ImportedMeasure measure;
                    measure.number = number++;
                    measure.numerator = numerator;
                    measure.denominator = denominator;
                    measure.startTick = startTick;
                    measure.durationTicks = *durationTicks;
                    measure.pickup = hasPickup && measure.number == 1;
                    startTick += measure.durationTicks;
                    ParsedMasterBar master{measure, splitText(childText(node, "Bars"))};
                    masterBars_.push_back(master);
                    song.measures.push_back(measure);
                }
            }

            std::optional<Fraction> voiceDuration(const ParsedVoice &voice) const
            {
                Fraction duration{};
                for (const std::string &beatId : voice.beatIds)
                {
                    const auto beat = beats_.find(beatId);
                    if (beat == beats_.end())
                    {
                        return std::nullopt;
                    }
                    const auto rhythm = rhythms_.find(beat->second.rhythmId);
                    if (rhythm == rhythms_.end() || !rhythm->second.valid)
                    {
                        return std::nullopt;
                    }
                    const auto advanced = add(duration, rhythm->second.duration);
                    if (!advanced.has_value())
                    {
                        return std::nullopt;
                    }
                    duration = *advanced;
                }
                return duration;
            }

            void adjustPickupMeasureTiming(ImportedSong &song)
            {
                if (masterBars_.empty() || !masterBars_.front().measure.pickup)
                {
                    return;
                }

                Fraction longest{};
                bool foundVoice = false;
                for (const std::string &barId : masterBars_.front().barIds)
                {
                    const auto bar = bars_.find(barId);
                    if (bar == bars_.end())
                    {
                        continue;
                    }
                    for (const std::string &voiceId : bar->second.voiceIds)
                    {
                        const auto voice = voices_.find(voiceId);
                        if (voice == voices_.end())
                        {
                            continue;
                        }
                        const auto duration = voiceDuration(voice->second);
                        if (!duration.has_value())
                        {
                            pickupTimingWarning_ = "Pickup duration uses nominal meter because its rhythm data is unsupported";
                            return;
                        }
                        if (!foundVoice ||
                            duration->numerator * longest.denominator >
                                longest.numerator * duration->denominator)
                        {
                            longest = *duration;
                        }
                        foundVoice = true;
                    }
                }
                if (!foundVoice || longest.numerator <= 0)
                {
                    pickupTimingWarning_ = "Pickup duration uses nominal meter because it has no readable rhythmic voice";
                    return;
                }

                if (longest.denominator != 1)
                {
                    pickupTimingWarning_ = "Pickup duration rounded to integer ticks";
                }
                masterBars_.front().measure.durationTicks = std::max(1, roundTicks(longest));
                int startTick = 0;
                for (std::size_t index = 0; index < masterBars_.size(); ++index)
                {
                    masterBars_[index].measure.startTick = startTick;
                    song.measures[index] = masterBars_[index].measure;
                    startTick += masterBars_[index].measure.durationTicks;
                }
            }

            void readRhythms()
            {
                for (const pugi::xml_node node : root_.child("Rhythms").children("Rhythm"))
                {
                    Rhythm rhythm;
                    const auto duration = noteValueDuration(childText(node, "NoteValue"));
                    if (!duration.has_value())
                    {
                        rhythms_[node.attribute("id").as_string()] = rhythm;
                        continue;
                    }
                    rhythm.duration = *duration;
                    const int dots = std::max(0, node.child("AugmentationDot").attribute("count").as_int(0));
                    if (dots > 0 && dots <= 3)
                    {
                        rhythm.duration = multiply(rhythm.duration, (1LL << (dots + 1)) - 1, 1LL << dots);
                    }
                    else if (dots > 3)
                    {
                        rhythms_[node.attribute("id").as_string()] = rhythm;
                        continue;
                    }
                    const pugi::xml_node tuplet = node.child("PrimaryTuplet");
                    if (tuplet)
                    {
                        const int num = tuplet.attribute("num").as_int(0);
                        const int den = tuplet.attribute("den").as_int(0);
                        if (num <= 0 || num > 1024 || den <= 0 || den > 1024)
                        {
                            rhythms_[node.attribute("id").as_string()] = rhythm;
                            continue;
                        }
                        rhythm.duration = multiply(rhythm.duration, den, num);
                    }
                    rhythm.valid = true;
                    rhythm.rounded = rhythm.duration.denominator != 1;
                    rhythms_[node.attribute("id").as_string()] = rhythm;
                }
            }

            void readNotes()
            {
                for (const pugi::xml_node node : root_.child("Notes").children("Note"))
                {
                    noteNodes_[node.attribute("id").as_string()] = node;
                }
            }

            void readBeats()
            {
                for (const pugi::xml_node node : root_.child("Beats").children("Beat"))
                {
                    ParsedBeat beat;
                    beat.rhythmId = node.child("Rhythm").attribute("ref").as_string();
                    beat.noteIds = splitText(childText(node, "Notes"));
                    beat.technique = parseBeatTechnique(node);
                    beats_[node.attribute("id").as_string()] = std::move(beat);
                }
            }

            void readVoices()
            {
                for (const pugi::xml_node node : root_.child("Voices").children("Voice"))
                {
                    voices_[node.attribute("id").as_string()] = {splitText(childText(node, "Beats"))};
                }
            }

            void readBars()
            {
                for (const pugi::xml_node node : root_.child("Bars").children("Bar"))
                {
                    bars_[node.attribute("id").as_string()] = {splitText(childText(node, "Voices"))};
                }
            }

            void readTracks()
            {
                std::unordered_map<std::string, ParsedTrack> byId;
                for (const pugi::xml_node node : root_.child("Tracks").children("Track"))
                {
                    ParsedTrack track;
                    track.id = node.attribute("id").as_string();
                    track.part.name = childText(node, "Name");
                    const pugi::xml_node instrumentSet = node.child("InstrumentSet");
                    track.part.instrumentName = childText(instrumentSet, "Name");
                    if (track.part.instrumentName.empty())
                    {
                        track.part.instrumentName = node.child("Instrument").attribute("ref").as_string();
                    }
                    pugi::xml_node properties = node.child("Properties");
                    if (!properties)
                    {
                        properties = node.child("Staves").child("Staff").child("Properties");
                    }
                    for (const pugi::xml_node property : properties.children("Property"))
                    {
                        if (std::string_view(property.attribute("name").as_string()) == "Tuning")
                        {
                            std::vector<std::string> pitches = splitText(childText(property, "Pitches"));
                            for (auto pitch = pitches.rbegin(); pitch != pitches.rend(); ++pitch)
                            {
                                if (const auto midi = parseInt(*pitch))
                                {
                                    track.part.suggestedTuning.push_back(
                                        openchordix::track::tuningNoteLabelFromMidi(*midi));
                                }
                            }
                        }
                    }
                    track.part.detectedStringCount = static_cast<int>(track.part.suggestedTuning.size());
                    track.part.suggestedStringCount = std::max(1, track.part.detectedStringCount);
                    track.part.containsTabMapping = track.part.detectedStringCount > 0;
                    pugi::xml_node midi = node.child("GeneralMidi") ? node.child("GeneralMidi") :
                                          node.child("MIDISettings");
                    if (!midi)
                    {
                        midi = node.child("Sounds").child("Sound").child("MIDI");
                    }
                    const bool percussion = lowercase(midi.attribute("table").as_string()) == "percussion";
                    track.part.midiProgram = parseInt(childText(midi, "Program"));
                    classify(track.part, percussion);
                    byId[track.id] = std::move(track);
                }

                const std::vector<std::string> order = splitText(childText(root_.child("MasterTrack"), "Tracks"));
                if (order.empty())
                {
                    for (auto &entry : byId)
                    {
                        tracks_.push_back(std::move(entry.second));
                    }
                    std::sort(tracks_.begin(), tracks_.end(),
                              [](const ParsedTrack &left, const ParsedTrack &right)
                              { return left.id < right.id; });
                    return;
                }
                for (const std::string &id : order)
                {
                    auto found = byId.find(id);
                    if (found != byId.end())
                    {
                        tracks_.push_back(std::move(found->second));
                    }
                }
            }

            void classify(ImportedPart &part, bool percussion) const
            {
                const std::string searchable = lowercase(part.name + " " + part.instrumentName);
                const int program = part.midiProgram.value_or(-1);
                if (percussion || searchable.find("drum") != std::string::npos ||
                    searchable.find("percussion") != std::string::npos)
                {
                    part.type = ImportedPartType::Percussion;
                    part.importByDefault = false;
                    part.status = "Percussion track; disabled by default";
                }
                else if (searchable.find("bass") != std::string::npos || (program >= 32 && program <= 39))
                {
                    part.type = ImportedPartType::Bass;
                    part.importByDefault = part.containsTabMapping;
                    part.status = part.containsTabMapping ? "Tablature available" : "No tablature mapping";
                }
                else if (searchable.find("guitar") != std::string::npos || (program >= 24 && program <= 31))
                {
                    part.type = ImportedPartType::Guitar;
                    part.importByDefault = part.containsTabMapping;
                    part.status = part.containsTabMapping ? "Tablature available" : "No tablature mapping";
                }
                else if (part.containsTabMapping)
                {
                    part.type = ImportedPartType::FrettedStrings;
                    part.importByDefault = true;
                    part.status = "Tablature available";
                }
                else
                {
                    part.type = ImportedPartType::Unknown;
                    part.importByDefault = false;
                    part.status = "No tablature mapping";
                }
            }

            void readTempo(ImportedSong &song) const
            {
                for (const pugi::xml_node automation : root_.child("MasterTrack").child("Automations").children("Automation"))
                {
                    if (childText(automation, "Type") != "Tempo")
                    {
                        continue;
                    }
                    const std::vector<std::string> values = splitText(childText(automation, "Value"));
                    if (values.empty())
                    {
                        continue;
                    }
                    const auto bpm = parseDouble(values.front());
                    const int bar = parseInt(childText(automation, "Bar")).value_or(0);
                    if (!bpm.has_value() || *bpm <= 0.0 || bar < 0 ||
                        bar >= static_cast<int>(song.measures.size()))
                    {
                        continue;
                    }
                    const ImportedMeasure &measure = song.measures[static_cast<std::size_t>(bar)];
                    const double position = parseDouble(childText(automation, "Position")).value_or(0.0);
                    const int offset = position > 0.0 && position <= 1.0
                                           ? static_cast<int>(std::llround(position * measure.durationTicks))
                                           : static_cast<int>(std::llround(std::max(0.0, position)));
                    song.tempos.push_back({measure.startTick + std::clamp(offset, 0, measure.durationTicks), *bpm});
                }
                if (song.tempos.empty())
                {
                    song.tempos.push_back({0, 120.0});
                }
                std::sort(song.tempos.begin(), song.tempos.end(),
                          [](const TempoEvent &left, const TempoEvent &right)
                          { return left.tick < right.tick; });
                song.tempos.erase(std::unique(song.tempos.begin(), song.tempos.end(),
                                              [](const TempoEvent &left, const TempoEvent &right)
                                              {
                                                  return left.tick == right.tick &&
                                                         left.beatsPerMinute == right.beatsPerMinute;
                                              }),
                                  song.tempos.end());
            }

            openchordix::track::TempoMap tempoMap(const ImportedSong &song) const
            {
                std::vector<openchordix::track::TempoEvent> events;
                events.reserve(song.tempos.size());
                for (const TempoEvent &tempo : song.tempos)
                {
                    events.push_back({tempo.tick, tempo.beatsPerMinute, "gpif"});
                }
                return openchordix::track::TempoMap(song.ticksPerBeat, std::move(events));
            }

            void materializeParts(ImportedSong &song)
            {
                for (std::size_t trackIndex = 0; trackIndex < tracks_.size(); ++trackIndex)
                {
                    ImportedPart &part = tracks_[trackIndex].part;
                    part.measures = song.measures;
                    if (!pickupTimingWarning_.empty())
                    {
                        addWarning(part, pickupTimingWarning_);
                    }
                    for (const std::string &warning : timingWarnings_)
                    {
                        addWarning(part, warning);
                    }
                    for (std::size_t measureIndex = 0; measureIndex < masterBars_.size(); ++measureIndex)
                    {
                        const ParsedMasterBar &master = masterBars_[measureIndex];
                        if (trackIndex >= master.barIds.size() || master.barIds[trackIndex] == "-1")
                        {
                            continue;
                        }
                        const auto bar = bars_.find(master.barIds[trackIndex]);
                        if (bar == bars_.end())
                        {
                            addWarning(part, "Missing bar data");
                            continue;
                        }
                        for (const std::string &voiceId : bar->second.voiceIds)
                        {
                            if (voiceId == "-1")
                            {
                                continue;
                            }
                            const auto voice = voices_.find(voiceId);
                            if (voice == voices_.end())
                            {
                                addWarning(part, "Missing voice data");
                                continue;
                            }
                            materializeVoice(part, voice->second, master.measure.startTick);
                        }
                    }
                    if (part.notes.empty())
                    {
                        part.importByDefault = false;
                        addWarning(part, "Empty track; disabled by default");
                    }
                    song.parts.push_back(std::move(part));
                }
            }

            void materializeVoice(ImportedPart &part, const ParsedVoice &voice, int measureStart)
            {
                Fraction offset{};
                for (const std::string &beatId : voice.beatIds)
                {
                    const auto beat = beats_.find(beatId);
                    if (beat == beats_.end())
                    {
                        addWarning(part, "Skipped voice with missing beat data");
                        return;
                    }
                    const auto rhythm = rhythms_.find(beat->second.rhythmId);
                    if (rhythm == rhythms_.end() || !rhythm->second.valid)
                    {
                        addWarning(part, "Skipped voice with unsupported rhythm data");
                        return;
                    }
                    if (rhythm->second.rounded)
                    {
                        addWarning(part, "Tuplet timing rounded to integer ticks");
                    }
                    const int tick = measureStart + roundTicks(offset);
                    const int duration = roundTicks(rhythm->second.duration);
                    for (const std::string &noteId : beat->second.noteIds)
                    {
                        const auto noteNode = noteNodes_.find(noteId);
                        if (noteNode == noteNodes_.end())
                        {
                            addWarning(part, "Missing note data");
                            continue;
                        }
                        const auto stringValue = propertyText(noteNode->second, "String", "String");
                        const auto fretValue = propertyText(noteNode->second, "Fret", "Fret");
                        const auto rawString = stringValue.has_value() ? parseInt(*stringValue) : std::nullopt;
                        const auto fret = fretValue.has_value() ? parseInt(*fretValue) : std::nullopt;
                        if (!rawString.has_value() || !fret.has_value() ||
                            *rawString < 0 || *rawString >= part.detectedStringCount)
                        {
                            addWarning(part, "Skipped note without tablature position");
                            continue;
                        }
                        ImportedNote note;
                        note.tick = tick;
                        note.duration = duration;
                        note.stringIndex = part.detectedStringCount - 1 - *rawString;
                        note.fret = std::max(0, *fret);
                        note.technique = parseNoteTechnique(noteNode->second);
                        mergeBeatTechnique(note.technique, beat->second.technique);
                        part.notes.push_back(std::move(note));
                    }
                    const auto advanced = add(offset, rhythm->second.duration);
                    if (!advanced.has_value())
                    {
                        addWarning(part, "Skipped voice with unsupported timing precision");
                        return;
                    }
                    offset = *advanced;
                }
            }

            pugi::xml_node root_;
            std::filesystem::path sourcePath_;
            std::string detectedFormat_;
            std::vector<ParsedMasterBar> masterBars_;
            std::vector<ParsedTrack> tracks_;
            std::unordered_map<std::string, Rhythm> rhythms_;
            std::unordered_map<std::string, pugi::xml_node> noteNodes_;
            std::unordered_map<std::string, ParsedBeat> beats_;
            std::unordered_map<std::string, ParsedVoice> voices_;
            std::unordered_map<std::string, ParsedBar> bars_;
            std::string pickupTimingWarning_;
            std::vector<std::string> timingWarnings_;
        };
    }

    ImportResult<ImportedSong> GuitarPro7Importer::importBytes(std::span<const std::uint8_t> bytes,
                                                                const std::filesystem::path &sourcePath,
                                                                const ImportOptions &) const
    {
        const auto documentResult = readScoreDocument(bytes);
        if (!documentResult)
        {
            return ImportResult<ImportedSong>::failure(documentResult.error());
        }

        pugi::xml_document xml;
        const pugi::xml_parse_result parseResult =
            xml.load_buffer(documentResult.value().data(), documentResult.value().size(),
                            pugi::parse_default | pugi::parse_ws_pcdata_single);
        if (!parseResult)
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::MalformedData,
                "The score.gpif XML document is malformed.",
                "ZIP package (score.gpif found)",
            });
        }
        const pugi::xml_node root = xml.child("GPIF");
        if (!root)
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::UnsupportedFormat,
                "The ZIP score document is not readable GPIF content.",
                "ZIP archive",
            });
        }
        const std::string versionText = childText(root, "GPVersion");
        const int version = parseMajorVersion(versionText).value_or(-1);
        if (version < 7)
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::UnsupportedFeature,
                "Readable GPIF score document has no supported Guitar Pro 7+ version marker.",
                "Guitar Pro GPIF package (unsupported version)",
            });
        }
        const std::string detected = "Guitar Pro " + versionText + " (zip package)";
        return GpifAdapter(root, sourcePath, detected).convert();
    }
}
