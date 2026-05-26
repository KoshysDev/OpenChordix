#include "track/import/GuitarProImporter.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include "gp_parser.h"
#include "track/TuningLibrary.h"
#include "track/import/GuitarPro7Importer.h"

namespace openchordix::track::imports
{
    namespace
    {
        constexpr int kGpTicksPerBeat = gp_parser::QUARTER_TIME;

        enum class GuitarProContent
        {
            Unknown,
            Gp3Or4,
            Gp5,
            Gpx,
            ZipPackageCandidate
        };

        struct GuitarProDetection
        {
            GuitarProContent content = GuitarProContent::Unknown;
            std::string label;
        };

        std::string lowerExtension(const std::filesystem::path &path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char value)
                           { return static_cast<char>(std::tolower(value)); });
            return extension;
        }

        bool beginsWith(std::span<const std::uint8_t> bytes, std::string_view prefix)
        {
            return bytes.size() >= prefix.size() &&
                   std::equal(prefix.begin(), prefix.end(), bytes.begin());
        }

        std::string gpHeader(std::span<const std::uint8_t> bytes)
        {
            if (bytes.empty() || bytes.front() > 64 ||
                bytes.size() < static_cast<std::size_t>(bytes.front()) + 1)
            {
                return {};
            }
            return std::string(reinterpret_cast<const char *>(bytes.data() + 1), bytes.front());
        }

        GuitarProDetection detectGuitarPro(std::span<const std::uint8_t> bytes,
                                           const std::filesystem::path &sourcePath)
        {
            if (beginsWith(bytes, "PK\x03\x04") ||
                beginsWith(bytes, "PK\x05\x06") ||
                beginsWith(bytes, "PK\x01\x02"))
            {
                return {GuitarProContent::ZipPackageCandidate, "ZIP archive"};
            }
            if (beginsWith(bytes, "BCFS") || beginsWith(bytes, "BCFZ"))
            {
                return {GuitarProContent::Gpx, "Guitar Pro 6 (GPX)"};
            }

            const std::string header = gpHeader(bytes);
            if (header.find("FICHIER GUITAR PRO v5.00") == 0)
            {
                return {GuitarProContent::Gp5, "Guitar Pro 5.00"};
            }
            if (header.find("FICHIER GUITAR PRO v5.10") == 0)
            {
                return {GuitarProContent::Gp5, "Guitar Pro 5.10"};
            }
            if (header.find("FICHIER GUITAR PRO v3.") == 0)
            {
                return {GuitarProContent::Gp3Or4, "Guitar Pro 3"};
            }
            if (header.find("FICHIER GUITAR PRO v4.") == 0)
            {
                return {GuitarProContent::Gp3Or4, "Guitar Pro 4"};
            }
            if (header.find("FICHIER GUITAR PRO") == 0)
            {
                return {GuitarProContent::Unknown, "Guitar Pro (unsupported version)"};
            }

            const std::string extension = lowerExtension(sourcePath);
            if (extension == ".gpx")
            {
                return {GuitarProContent::Gpx, "Guitar Pro 6 (GPX)"};
            }
            return {GuitarProContent::Unknown, "Guitar Pro (unrecognized content)"};
        }

        ImportResult<ImportedSong> unsupported(const GuitarProDetection &detection,
                                               std::string message)
        {
            return ImportResult<ImportedSong>::failure(
                {ImportErrorCode::UnsupportedFeature, std::move(message), detection.label});
        }

        int importedTick(int gpTick)
        {
            return std::max(0, gpTick - kGpTicksPerBeat);
        }

        int importedDuration(double ticks)
        {
            return std::max(1, static_cast<int>(std::llround(ticks)));
        }

        int measureDuration(const gp_parser::MeasureHeader &header)
        {
            const int numerator = std::max(1, static_cast<int>(header.timeSignature.numerator));
            const int denominator = std::max(1, static_cast<int>(header.timeSignature.denominator.value));
            const std::int64_t scaled = static_cast<std::int64_t>(numerator) *
                                        static_cast<std::int64_t>(kGpTicksPerBeat) * 4;
            return std::max(1, static_cast<int>((scaled + denominator / 2) / denominator));
        }

        std::string lowercase(std::string text)
        {
            std::transform(text.begin(), text.end(), text.begin(),
                           [](unsigned char value)
                           { return static_cast<char>(std::tolower(value)); });
            return text;
        }

        const gp_parser::Channel *trackChannel(const gp_parser::TabFile &file,
                                               const gp_parser::Track &track)
        {
            const auto found = std::find_if(file.channels.begin(), file.channels.end(),
                                            [&](const gp_parser::Channel &channel)
                                            { return channel.id == track.channelId; });
            return found == file.channels.end() ? nullptr : &*found;
        }

        void classifyPart(ImportedPart &part, const gp_parser::TabFile &file,
                          const gp_parser::Track &track)
        {
            const gp_parser::Channel *channel = trackChannel(file, track);
            const int program = channel == nullptr ? -1 : channel->program;
            const bool percussion = channel != nullptr && channel->isPercussionChannel;
            part.midiProgram = program >= 0 ? std::optional<int>(program) : std::nullopt;

            const std::string name = lowercase(track.name);
            if (percussion)
            {
                part.type = ImportedPartType::Percussion;
                part.instrumentName = "Percussion";
                part.importByDefault = false;
                part.status = "Percussion track; disabled by default";
            }
            else if (name.find("bass") != std::string::npos || (program >= 32 && program <= 39))
            {
                part.type = ImportedPartType::Bass;
                part.instrumentName = "Bass";
            }
            else if (name.find("guitar") != std::string::npos || (program >= 24 && program <= 31))
            {
                part.type = ImportedPartType::Guitar;
                part.instrumentName = "Guitar";
            }
            else if (!track.strings.empty())
            {
                part.type = ImportedPartType::FrettedStrings;
                part.instrumentName = "Fretted strings";
            }
            else
            {
                part.type = ImportedPartType::Unknown;
                part.instrumentName = "Unknown";
                part.importByDefault = false;
            }

            if (!percussion && !track.strings.empty())
            {
                part.importByDefault = true;
                part.status = "Tablature available";
            }
        }

        ImportedTechnique importTechnique(const gp_parser::Note &note)
        {
            const gp_parser::NoteEffect &effect = note.effect;
            ImportedTechnique technique;
            technique.noteType = note.tiedNote ? "tie" : (effect.deadNote ? "dead" : "normal");
            technique.slideType = effect.slide ? "slide" : "";
            technique.harmonicType = effect.harmonic.type;
            technique.hammerOn = effect.hammer;
            technique.bend = !effect.bend.points.empty();
            technique.vibrato = effect.vibrato;
            technique.palmMute = effect.palmMute;
            technique.letRing = effect.letRing;
            technique.staccato = effect.staccato;
            technique.tremoloPicking = !effect.tremoloPicking.duration.value.empty();
            technique.trill = !effect.trill.duration.value.empty();
            technique.accent = effect.accentuatedNote;
            technique.heavyAccent = effect.heavyAccentuatedNote;
            if (effect.tapping)
            {
                technique.pluckStyle = "tap";
            }
            else if (effect.slapping)
            {
                technique.pluckStyle = "slap";
            }
            else if (effect.popping)
            {
                technique.pluckStyle = "pop";
            }
            return technique;
        }

        void addNotes(ImportedPart &part, const gp_parser::Track &track)
        {
            for (const gp_parser::Measure &measure : track.measures)
            {
                for (const gp_parser::Beat &beat : measure.beats)
                {
                    for (const gp_parser::Voice &voice : beat.voices)
                    {
                        for (const gp_parser::Note &source : voice.notes)
                        {
                            if (source.string < 1 ||
                                source.string > static_cast<int>(track.strings.size()))
                            {
                                continue;
                            }
                            const gp_parser::GuitarString &string =
                                track.strings[static_cast<std::size_t>(source.string - 1)];
                            ImportedNote note;
                            note.tick = importedTick(beat.start);
                            note.duration = importedDuration(voice.duration);
                            note.midiPitch = string.value + source.value;
                            note.velocity = source.velocity;
                            note.stringIndex = source.string - 1;
                            note.fret = static_cast<int>(source.value);
                            note.technique = importTechnique(source);
                            part.notes.push_back(std::move(note));
                        }
                    }
                }
            }
            std::sort(part.notes.begin(), part.notes.end(),
                      [](const ImportedNote &left, const ImportedNote &right)
                      {
                          if (left.tick != right.tick)
                          {
                              return left.tick < right.tick;
                          }
                          return left.stringIndex.value_or(0) < right.stringIndex.value_or(0);
                      });
        }

        void calculateDurationSeconds(ImportedSong &song)
        {
            double bpm = song.tempos.empty() ? 120.0 : song.tempos.front().beatsPerMinute;
            int precedingTick = 0;
            for (const TempoEvent &tempo : song.tempos)
            {
                const int tick = std::clamp(tempo.tick, precedingTick, song.durationTicks);
                song.durationSeconds += static_cast<double>(tick - precedingTick) /
                                        static_cast<double>(song.ticksPerBeat) * 60.0 / bpm;
                precedingTick = tick;
                if (tempo.beatsPerMinute > 0.0)
                {
                    bpm = tempo.beatsPerMinute;
                }
            }
            song.durationSeconds += static_cast<double>(song.durationTicks - precedingTick) /
                                    static_cast<double>(song.ticksPerBeat) * 60.0 / bpm;
        }

        ImportedSong convertFile(const gp_parser::TabFile &file,
                                 const std::filesystem::path &path,
                                 std::string detectedFormat)
        {
            ImportedSong song;
            song.format = ImportFormat::GuitarPro;
            song.sourcePath = path;
            song.detectedFormat = std::move(detectedFormat);
            song.title = file.title;
            song.subtitle = file.subtitle;
            song.artist = file.artist;
            song.album = file.album;
            song.ticksPerBeat = kGpTicksPerBeat;

            for (const gp_parser::MeasureHeader &header : file.measureHeaders)
            {
                ImportedMeasure measure;
                measure.number = header.number;
                measure.numerator = std::max(1, static_cast<int>(header.timeSignature.numerator));
                measure.denominator = std::max(1, static_cast<int>(header.timeSignature.denominator.value));
                measure.startTick = importedTick(header.start);
                measure.durationTicks = measureDuration(header);
                song.measures.push_back(measure);
            }
            if (!song.measures.empty())
            {
                const ImportedMeasure &last = song.measures.back();
                song.durationTicks = last.startTick + last.durationTicks;
            }

            const double baseTempo = file.tempoValue > 0 ? static_cast<double>(file.tempoValue) : 120.0;
            song.tempos.push_back({0, baseTempo});
            for (std::size_t index = 0; index < file.measureHeaders.size(); ++index)
            {
                const int value = file.measureHeaders[index].tempo.value;
                const int tick = index < song.measures.size() ? song.measures[index].startTick : 0;
                if (value > 0 && (song.tempos.back().beatsPerMinute != value ||
                                  song.tempos.back().tick != tick))
                {
                    song.tempos.push_back({tick, static_cast<double>(value)});
                }
            }

            song.parts.reserve(file.tracks.size());
            for (const gp_parser::Track &track : file.tracks)
            {
                ImportedPart part;
                part.name = track.name;
                part.detectedStringCount = static_cast<int>(track.strings.size());
                part.suggestedStringCount = std::max(1, part.detectedStringCount);
                part.containsTabMapping = !track.strings.empty();
                part.measures = song.measures;
                for (const gp_parser::GuitarString &string : track.strings)
                {
                    part.suggestedTuning.push_back(
                        openchordix::track::tuningNoteLabelFromMidi(string.value));
                }
                classifyPart(part, file, track);
                addNotes(part, track);
                if (part.notes.empty())
                {
                    part.importByDefault = false;
                    part.status = "Empty track; disabled by default";
                }
                song.parts.push_back(std::move(part));
            }

            calculateDurationSeconds(song);
            return song;
        }
    }

    ImportResult<ImportedSong> GuitarProImporter::importFile(const std::filesystem::path &path,
                                                              const ImportOptions &options) const
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.good())
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::FileReadFailed,
                "Could not open Guitar Pro file '" + path.string() + "'.",
            });
        }
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                        std::istreambuf_iterator<char>());
        return importBytes(std::span<const std::uint8_t>(bytes), path, options);
    }

    ImportResult<ImportedSong> GuitarProImporter::importBytes(std::span<const std::uint8_t> bytes,
                                                               const std::filesystem::path &sourcePath,
                                                               const ImportOptions &) const
    {
        const GuitarProDetection detection = detectGuitarPro(bytes, sourcePath);
        switch (detection.content)
        {
        case GuitarProContent::Gp3Or4:
            return unsupported(detection,
                               detection.label + " files are recognized but are not supported by the GP5 importer.");
        case GuitarProContent::Gpx:
            return unsupported(detection,
                               "Guitar Pro 6 GPX files are recognized but are not supported by the GP5 importer.");
        case GuitarProContent::ZipPackageCandidate:
            return GuitarPro7Importer().importBytes(bytes, sourcePath);
        case GuitarProContent::Unknown:
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::MalformedData,
                "The selected file does not contain a recognized supported Guitar Pro 5 header.",
                detection.label,
            });
        case GuitarProContent::Gp5:
            break;
        }

        try
        {
            gp_parser::Parser parser(bytes);
            return ImportResult<ImportedSong>::success(
                convertFile(parser.getTabFile(), sourcePath, detection.label));
        }
        catch (const std::exception &error)
        {
            return ImportResult<ImportedSong>::failure({
                ImportErrorCode::MalformedData,
                "Failed to parse " + detection.label + " file: " + error.what() + ".",
                detection.label,
            });
        }
    }
}
