#include "track/import/ImportTimingDebug.h"

#include <algorithm>
#include <sstream>

namespace openchordix::track::imports
{
    namespace
    {
        int measureIndexAtTick(const std::vector<ImportedMeasure> &measures, int tick)
        {
            for (std::size_t index = 0; index < measures.size(); ++index)
            {
                const ImportedMeasure &measure = measures[index];
                if (tick >= measure.startTick &&
                    tick < measure.startTick + measure.durationTicks)
                {
                    return static_cast<int>(index);
                }
            }
            return -1;
        }

        std::string techniqueText(const ImportedTechnique &technique)
        {
            std::vector<std::string> labels;
            const auto add = [&](bool value, const char *label)
            {
                if (value)
                {
                    labels.emplace_back(label);
                }
            };
            add(technique.hammerOn, "hammer");
            add(technique.pullOff, "pull");
            add(technique.bend, "bend");
            add(technique.vibrato, "vibrato");
            add(technique.palmMute, "palm-mute");
            add(technique.letRing, "let-ring");
            add(technique.staccato, "staccato");
            add(technique.tremoloPicking, "tremolo");
            add(technique.trill, "trill");
            add(technique.accent, "accent");
            add(technique.heavyAccent, "heavy-accent");
            if (!technique.slideType.empty())
            {
                labels.push_back("slide:" + technique.slideType);
            }
            if (!technique.harmonicType.empty())
            {
                labels.push_back("harmonic:" + technique.harmonicType);
            }
            if (!technique.pluckStyle.empty())
            {
                labels.push_back(technique.pluckStyle);
            }
            std::ostringstream text;
            for (std::size_t index = 0; index < labels.size(); ++index)
            {
                if (index != 0)
                {
                    text << ", ";
                }
                text << labels[index];
            }
            return labels.empty() ? "none" : text.str();
        }
    }

    ImportTimingDebugSummary buildTimingDebugSummary(const ImportedSong &song,
                                                      std::size_t maximumNotesPerPart,
                                                      std::size_t maximumMeasures)
    {
        ImportTimingDebugSummary summary;
        summary.ticksPerBeat = song.ticksPerBeat;
        summary.measureCount = song.measures.size();
        if (!song.tempos.empty())
        {
            summary.initialTempo = song.tempos.front().beatsPerMinute;
        }
        summary.measures.assign(song.measures.begin(),
                                song.measures.begin() + std::min(maximumMeasures, song.measures.size()));

        for (const ImportedPart &part : song.parts)
        {
            ImportTimingPartDebug partDebug;
            partDebug.name = part.name;
            std::vector<ImportedNote> notes = part.notes;
            std::sort(notes.begin(), notes.end(),
                      [](const ImportedNote &left, const ImportedNote &right)
                      {
                          if (left.tick != right.tick)
                          {
                              return left.tick < right.tick;
                          }
                          return left.stringIndex.value_or(-1) < right.stringIndex.value_or(-1);
                      });
            if (!notes.empty())
            {
                partDebug.firstNoteTick = notes.front().tick;
                partDebug.firstNoteMeasureIndex = measureIndexAtTick(song.measures, notes.front().tick);
                partDebug.firstNonEmptyMeasureIndex = partDebug.firstNoteMeasureIndex;
            }
            for (std::size_t index = 0; index < std::min(maximumNotesPerPart, notes.size()); ++index)
            {
                const ImportedNote &note = notes[index];
                const int measureIndex = measureIndexAtTick(song.measures, note.tick);
                const int measureStart = measureIndex >= 0
                                             ? song.measures[static_cast<std::size_t>(measureIndex)].startTick
                                             : 0;
                partDebug.notes.push_back({
                    measureIndex,
                    note.tick - measureStart,
                    note.tick,
                    note.duration,
                    note.stringIndex.value_or(-1),
                    note.fret.value_or(-1),
                    techniqueText(note.technique),
                });
            }
            summary.parts.push_back(std::move(partDebug));
        }
        return summary;
    }
}
