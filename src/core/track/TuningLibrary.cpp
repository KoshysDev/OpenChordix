#include "TuningLibrary.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <exception>
#include <fstream>
#include <system_error>

#include <nlohmann/json.hpp>

#include "AppPaths.h"
#include "TrackStringUtils.h"

using nlohmann::json;
using openchordix::track::trimCopy;

namespace
{
    struct ParsedTuningNote
    {
        int semitone = 0;
        int octave = 0;
        bool hasOctave = false;
    };

    constexpr std::array<const char *, 12> kPitchClassNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    int pitchClassFromLetter(char letter)
    {
        switch (static_cast<char>(std::toupper(static_cast<unsigned char>(letter))))
        {
        case 'C':
            return 0;
        case 'D':
            return 2;
        case 'E':
            return 4;
        case 'F':
            return 5;
        case 'G':
            return 7;
        case 'A':
            return 9;
        case 'B':
            return 11;
        default:
            return -1;
        }
    }

    std::optional<ParsedTuningNote> parseTuningNote(std::string_view raw)
    {
        const std::string trimmed = trimCopy(raw);
        if (trimmed.empty())
        {
            return std::nullopt;
        }

        const int base = pitchClassFromLetter(trimmed.front());
        if (base < 0)
        {
            return std::nullopt;
        }

        ParsedTuningNote parsed;
        parsed.semitone = base;

        size_t pos = 1;
        if (pos < trimmed.size())
        {
            const char accidental = trimmed[pos];
            if (accidental == '#' || accidental == 'b')
            {
                parsed.semitone += accidental == '#' ? 1 : -1;
                ++pos;
            }
        }

        if (pos < trimmed.size())
        {
            const std::string octaveText = trimmed.substr(pos);
            const bool validOctaveText = std::all_of(
                octaveText.begin(),
                octaveText.end(),
                [](unsigned char ch)
                {
                    return std::isdigit(ch) != 0 || ch == '-';
                });
            if (!validOctaveText)
            {
                return std::nullopt;
            }
            try
            {
                parsed.octave = std::stoi(octaveText);
                parsed.hasOctave = true;
            }
            catch (const std::exception &)
            {
                return std::nullopt;
            }
        }

        while (parsed.semitone < 0)
        {
            parsed.semitone += 12;
            if (parsed.hasOctave)
            {
                --parsed.octave;
            }
        }
        while (parsed.semitone >= 12)
        {
            parsed.semitone -= 12;
            if (parsed.hasOctave)
            {
                ++parsed.octave;
            }
        }

        return parsed;
    }

    bool notesContainImplicitOctaves(const std::vector<std::string> &notes)
    {
        for (const std::string &note : notes)
        {
            const auto parsed = parseTuningNote(note);
            if (parsed.has_value() && !parsed->hasOctave)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<openchordix::track::TuningPreset> builtInTunings()
    {
        using openchordix::track::TuningPreset;
        return {
            {"Standard E", {"E4", "B3", "G3", "D3", "A2", "E2"}},
            {"Drop D", {"E4", "B3", "G3", "D3", "A2", "D2"}},
            {"Eb Standard", {"D#4", "A#3", "F#3", "C#3", "G#2", "D#2"}},
            {"D Standard", {"D4", "A3", "F3", "C3", "G2", "D2"}},
            {"Drop C", {"D4", "A3", "F3", "C3", "G2", "C2"}},
            {"7-string Standard", {"E4", "B3", "G3", "D3", "A2", "E2", "B1"}},
            {"8-string Standard", {"E4", "B3", "G3", "D3", "A2", "E2", "B1", "F#1"}},
            {"Bass Standard", {"G2", "D2", "A1", "E1"}},
            {"5-string Bass", {"G2", "D2", "A1", "E1", "B0"}},
            {"6-string Bass", {"C3", "G2", "D2", "A1", "E1", "B0"}}};
    }

    std::vector<std::string> normalizePresetNotes(const std::vector<std::string> &notes)
    {
        std::vector<std::string> normalized;
        normalized.reserve(static_cast<size_t>(openchordix::track::clampTrackStringCount(static_cast<int>(notes.size()))));
        for (const std::string &note : notes)
        {
            const std::string normalizedNote = openchordix::track::normalizeTuningNote(note);
            if (!normalizedNote.empty())
            {
                normalized.push_back(normalizedNote);
            }
            if (static_cast<int>(normalized.size()) >= openchordix::track::kMaxTrackStrings)
            {
                break;
            }
        }
        return normalized;
    }

    void sortPresets(std::vector<openchordix::track::TuningPreset> &presets)
    {
        std::sort(
            presets.begin(),
            presets.end(),
            [](const auto &lhs, const auto &rhs)
            {
                if (lhs.notes.size() != rhs.notes.size())
                {
                    return lhs.notes.size() < rhs.notes.size();
                }
                return lhs.name < rhs.name;
            });
    }
}

namespace openchordix::track
{
    std::string normalizeTuningNote(std::string_view note)
    {
        const auto parsed = parseTuningNote(note);
        if (!parsed.has_value())
        {
            return trimCopy(note);
        }

        std::string normalized = kPitchClassNames[static_cast<size_t>(parsed->semitone)];
        if (parsed->hasOctave)
        {
            normalized += std::to_string(parsed->octave);
        }
        return normalized;
    }

    std::string displayTuningNote(std::string_view note)
    {
        const auto parsed = parseTuningNote(note);
        if (!parsed.has_value())
        {
            return trimCopy(note);
        }
        return kPitchClassNames[static_cast<size_t>(parsed->semitone)];
    }

    std::optional<int> tuningNoteMidi(std::string_view note)
    {
        const auto parsed = parseTuningNote(note);
        if (!parsed.has_value() || !parsed->hasOctave)
        {
            return std::nullopt;
        }

        const int midi = (parsed->octave + 1) * 12 + parsed->semitone;
        if (midi < 0 || midi > 127)
        {
            return std::nullopt;
        }
        return midi;
    }

    std::string tuningNoteLabelFromMidi(int midi)
    {
        if (midi < 0 || midi > 127)
        {
            return "---";
        }

        const int octave = midi / 12 - 1;
        std::string label = kPitchClassNames[static_cast<size_t>(midi % 12)];
        label += std::to_string(octave);
        return label;
    }

    double tuningFrequencyFromMidi(int midi)
    {
        return 440.0 * std::pow(2.0, (static_cast<double>(midi) - 69.0) / 12.0);
    }

    bool tuningNotesEqual(const std::vector<std::string> &lhs,
                          const std::vector<std::string> &rhs,
                          bool ignoreOctave)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (size_t i = 0; i < lhs.size(); ++i)
        {
            const std::string left = ignoreOctave ? displayTuningNote(lhs[i]) : normalizeTuningNote(lhs[i]);
            const std::string right = ignoreOctave ? displayTuningNote(rhs[i]) : normalizeTuningNote(rhs[i]);
            if (left != right)
            {
                return false;
            }
        }
        return true;
    }

    std::string tuningNotesSummary(const std::vector<std::string> &notes, bool stripOctave)
    {
        std::string summary;
        for (size_t i = 0; i < notes.size(); ++i)
        {
            if (!summary.empty())
            {
                summary += ' ';
            }
            summary += stripOctave ? displayTuningNote(notes[i]) : normalizeTuningNote(notes[i]);
        }
        return summary;
    }

    TuningLibrary::TuningLibrary()
        : storagePath_(openchordix::core::executableDirectory() / "tunings.json")
    {
        load();
    }

    std::vector<const TuningPreset *> TuningLibrary::presetsForStringCount(int stringCount) const
    {
        const int safeStringCount = clampTrackStringCount(stringCount);
        std::vector<const TuningPreset *> matches;
        for (const TuningPreset &preset : presets_)
        {
            if (static_cast<int>(preset.notes.size()) == safeStringCount)
            {
                matches.push_back(&preset);
            }
        }
        return matches;
    }

    const TuningPreset *TuningLibrary::findByNotes(const std::vector<std::string> &notes, bool ignoreOctave) const
    {
        const std::vector<std::string> normalized = normalizePresetNotes(notes);
        if (normalized.empty())
        {
            return nullptr;
        }

        for (const TuningPreset &preset : presets_)
        {
            if (tuningNotesEqual(preset.notes, normalized, ignoreOctave))
            {
                return &preset;
            }
        }
        return nullptr;
    }

    const TuningPreset *TuningLibrary::defaultPresetForPart(std::string_view partName, int stringCount) const
    {
        const auto matches = presetsForStringCount(stringCount);
        if (matches.empty())
        {
            return nullptr;
        }

        const bool bassLike = isBassLikePartName(partName);
        const char *preferredName = nullptr;
        if (bassLike)
        {
            preferredName = stringCount == 5 ? "5-string Bass" : (stringCount == 6 ? "6-string Bass" : "Bass Standard");
        }
        else
        {
            preferredName = stringCount == 7 ? "7-string Standard" : (stringCount == 8 ? "8-string Standard" : "Standard E");
        }

        const auto preferred = std::find_if(
            matches.begin(),
            matches.end(),
            [&](const TuningPreset *preset)
            {
                return preset->name == preferredName;
            });
        if (preferred != matches.end())
        {
            return *preferred;
        }
        return matches.front();
    }

    bool TuningLibrary::addPreset(const TuningPreset &preset)
    {
        const std::string name = trimCopy(preset.name);
        const std::vector<std::string> notes = normalizePresetNotes(preset.notes);
        if (name.empty() || notes.empty())
        {
            return false;
        }

        if (findByNotes(notes, false) != nullptr)
        {
            return true;
        }

        if (notesContainImplicitOctaves(notes) && findByNotes(notes, true) != nullptr)
        {
            return true;
        }

        presets_.push_back(TuningPreset{name, notes});
        sortPresets(presets_);
        return save();
    }

    bool TuningLibrary::ensurePreset(std::string_view suggestedName, const std::vector<std::string> &notes)
    {
        const std::vector<std::string> normalized = normalizePresetNotes(notes);
        if (normalized.empty())
        {
            return false;
        }
        if (findByNotes(normalized, false) != nullptr)
        {
            return true;
        }
        if (notesContainImplicitOctaves(normalized) && findByNotes(normalized, true) != nullptr)
        {
            return true;
        }

        std::string name = trimCopy(suggestedName);
        if (name.empty())
        {
            name = std::to_string(normalized.size()) + "-string Custom";
        }
        return addPreset(TuningPreset{name, normalized});
    }

    void TuningLibrary::load()
    {
        presets_.clear();
        for (const TuningPreset &preset : builtInTunings())
        {
            mergePreset(preset);
        }

        std::ifstream in(storagePath_);
        if (!in.good())
        {
            sortPresets(presets_);
            return;
        }

        json root;
        try
        {
            in >> root;
        }
        catch (const json::exception &)
        {
            sortPresets(presets_);
            return;
        }

        if (!root.is_array())
        {
            sortPresets(presets_);
            return;
        }

        for (const auto &entry : root)
        {
            if (!entry.is_object())
            {
                continue;
            }

            TuningPreset preset;
            preset.name = trimCopy(entry.value("name", ""));
            if (const auto notesIt = entry.find("notes"); notesIt != entry.end() && notesIt->is_array())
            {
                for (const auto &note : *notesIt)
                {
                    if (note.is_string())
                    {
                        preset.notes.push_back(note.get<std::string>());
                    }
                }
            }
            mergePreset(preset);
        }

        sortPresets(presets_);
    }

    bool TuningLibrary::save() const
    {
        std::error_code ec;
        std::filesystem::create_directories(storagePath_.parent_path(), ec);

        json root = json::array();
        for (const TuningPreset &preset : presets_)
        {
            root.push_back(
                {
                    {"name", preset.name},
                    {"notes", preset.notes},
                });
        }

        std::ofstream out(storagePath_, std::ios::trunc);
        if (!out.good())
        {
            return false;
        }

        out << root.dump(2);
        return out.good();
    }

    void TuningLibrary::mergePreset(const TuningPreset &preset)
    {
        const std::string name = trimCopy(preset.name);
        const std::vector<std::string> notes = normalizePresetNotes(preset.notes);
        if (name.empty() || notes.empty())
        {
            return;
        }

        const bool useLooseMatch = notesContainImplicitOctaves(notes);
        auto it = std::find_if(
            presets_.begin(),
            presets_.end(),
            [&](const TuningPreset &existing)
            {
                if (tuningNotesEqual(existing.notes, notes, false))
                {
                    return true;
                }
                return useLooseMatch && tuningNotesEqual(existing.notes, notes, true);
            });
        if (it == presets_.end())
        {
            presets_.push_back(TuningPreset{name, notes});
        }
    }
}
