#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "TrackTypes.h"

namespace openchordix::track
{
    struct TuningPreset
    {
        std::string name;
        std::vector<std::string> notes;
    };

    std::string normalizeTuningNote(std::string_view note);
    std::string displayTuningNote(std::string_view note);
    std::optional<int> tuningNoteMidi(std::string_view note);
    std::string tuningNoteLabelFromMidi(int midi);
    double tuningFrequencyFromMidi(int midi);
    bool tuningNotesEqual(const std::vector<std::string> &lhs,
                          const std::vector<std::string> &rhs,
                          bool ignoreOctave = false);
    std::string tuningNotesSummary(const std::vector<std::string> &notes, bool stripOctave = true);

    class TuningLibrary
    {
    public:
        TuningLibrary();

        const std::vector<TuningPreset> &presets() const { return presets_; }
        std::vector<const TuningPreset *> presetsForStringCount(int stringCount) const;
        const TuningPreset *findByNotes(const std::vector<std::string> &notes, bool ignoreOctave = false) const;
        const TuningPreset *defaultPresetForPart(std::string_view partName, int stringCount) const;
        bool addPreset(const TuningPreset &preset);
        bool ensurePreset(std::string_view suggestedName, const std::vector<std::string> &notes);
        std::filesystem::path storagePath() const { return storagePath_; }

    private:
        std::filesystem::path storagePath_;
        std::vector<TuningPreset> presets_;

        void load();
        bool save() const;
        void mergePreset(const TuningPreset &preset);
    };
}
