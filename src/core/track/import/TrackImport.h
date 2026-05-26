#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "track/TrackTypes.h"

namespace openchordix::track::imports
{
    enum class ImportFormat
    {
        Midi,
        GuitarPro
    };

    enum class ImportStage
    {
        Idle,
        SelectingFile,
        Loading,
        Parsing,
        Converting,
        ReadyToPreview,
        Applying,
        Complete,
        Failed
    };

    enum class ImportedPartType
    {
        Unknown,
        Guitar,
        Bass,
        FrettedStrings,
        Percussion
    };

    enum class ImportErrorCode
    {
        UnsupportedFormat,
        FileReadFailed,
        MalformedData,
        UnsupportedFeature,
        NoImportableParts
    };

    struct ImportError
    {
        ImportErrorCode code = ImportErrorCode::MalformedData;
        std::string message;
        std::string detectedFormat;
    };

    struct ImportProgress
    {
        ImportStage stage = ImportStage::Idle;
        float fraction = 0.0f;
        std::string status;
    };

    template <typename T>
    class ImportResult
    {
    public:
        static ImportResult success(T value)
        {
            return ImportResult(std::move(value));
        }

        static ImportResult failure(ImportError error)
        {
            return ImportResult(std::move(error));
        }

        bool hasValue() const { return std::holds_alternative<T>(value_); }
        explicit operator bool() const { return hasValue(); }

        T &value() { return std::get<T>(value_); }
        const T &value() const { return std::get<T>(value_); }
        const ImportError &error() const { return std::get<ImportError>(value_); }

    private:
        explicit ImportResult(T value) : value_(std::move(value)) {}
        explicit ImportResult(ImportError error) : value_(std::move(error)) {}

        std::variant<T, ImportError> value_;
    };

    struct ImportedTechnique
    {
        std::string noteType = "normal";
        std::string slideType;
        std::string harmonicType;
        std::string pluckStyle;
        bool hammerOn = false;
        bool pullOff = false;
        bool bend = false;
        bool vibrato = false;
        bool palmMute = false;
        bool letRing = false;
        bool staccato = false;
        bool tremoloPicking = false;
        bool trill = false;
        bool accent = false;
        bool heavyAccent = false;
    };

    struct ImportedNote
    {
        int tick = 0;
        int duration = 1;
        int midiPitch = -1;
        int velocity = 0;
        std::optional<int> stringIndex;
        std::optional<int> fret;
        ImportedTechnique technique;
    };

    struct ImportedMeasure
    {
        int number = 0;
        int numerator = 4;
        int denominator = 4;
        int startTick = 0;
        int durationTicks = 0;
        bool pickup = false;
    };

    struct ImportedPart
    {
        std::string name;
        std::string instrumentName;
        ImportedPartType type = ImportedPartType::Unknown;
        std::vector<ImportedNote> notes;
        std::vector<ImportedMeasure> measures;
        int detectedStringCount = openchordix::track::kDefaultTrackStrings;
        int suggestedStringCount = openchordix::track::kDefaultTrackStrings;
        std::vector<std::string> suggestedTuning;
        std::optional<int> midiProgram;
        bool containsTabMapping = false;
        bool importByDefault = true;
        std::string status;
    };

    struct TempoEvent
    {
        int tick = 0;
        double beatsPerMinute = 120.0;
    };

    struct ImportedSong
    {
        ImportFormat format = ImportFormat::Midi;
        std::filesystem::path sourcePath;
        std::string detectedFormat;
        std::string title;
        std::string subtitle;
        std::string artist;
        std::string album;
        int ticksPerBeat = 0;
        int durationTicks = 0;
        double durationSeconds = 0.0;
        std::vector<ImportedMeasure> measures;
        std::vector<TempoEvent> tempos;
        std::vector<ImportedPart> parts;
    };

    struct ImportOptions
    {
        bool includeEmptyParts = false;
    };

    inline const char *partTypeLabel(ImportedPartType type)
    {
        switch (type)
        {
        case ImportedPartType::Guitar:
            return "Guitar";
        case ImportedPartType::Bass:
            return "Bass";
        case ImportedPartType::FrettedStrings:
            return "Fretted Strings";
        case ImportedPartType::Percussion:
            return "Percussion";
        default:
            return "Unknown";
        }
    }
}
