#include "track/import/TrackImportApply.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace openchordix::track::imports
{
    namespace
    {
        int convertTick(int tick, int sourceTicksPerBeat, int destinationTicksPerBeat)
        {
            const std::int64_t source = std::max(1, sourceTicksPerBeat);
            const std::int64_t scaled = static_cast<std::int64_t>(std::max(0, tick)) *
                                        std::max(1, destinationTicksPerBeat);
            const std::int64_t rounded = (scaled + source / 2) / source;
            return static_cast<int>(std::min<std::int64_t>(
                rounded, std::numeric_limits<int>::max()));
        }

        int placementOffset(const TrackChartDocument &document, const ImportPlacementOptions &placement)
        {
            switch (placement.mode)
            {
            case ImportPlacement::CurrentCursor:
                return std::max(0, placement.currentCursorTick);
            case ImportPlacement::AppendAfterExisting:
                return document.timelineEndTick();
            case ImportPlacement::CustomTickOffset:
                return std::max(0, placement.customTickOffset);
            default:
                return 0;
            }
        }

        TrackChartMeasure convertMeasure(const ImportedMeasure &source, int sourceTicksPerBeat,
                                         int destinationTicksPerBeat, int offset)
        {
            return {
                source.number,
                source.numerator,
                source.denominator,
                convertTick(source.startTick, sourceTicksPerBeat, destinationTicksPerBeat) + offset,
                std::max(1, convertTick(source.durationTicks, sourceTicksPerBeat, destinationTicksPerBeat)),
                source.pickup,
            };
        }
    }

    ImportApplySummary applyImportedParts(TrackChartDocument &document,
                                          const ImportedSong &song,
                                          const std::vector<SelectedImportPart> &selections,
                                          ImportMergeMode mergeMode,
                                          const ImportPlacementOptions &placement)
    {
        ImportApplySummary summary;
        summary.placementOffsetTicks = placementOffset(document, placement);
        const bool hasSelectedPart = std::any_of(
            selections.begin(), selections.end(),
            [&](const SelectedImportPart &selection)
            { return selection.enabled && selection.sourceIndex < song.parts.size(); });
        if (mergeMode == ImportMergeMode::ReplaceAllNotes)
        {
            document.notes().clear();
            document.measures().clear();
        }

        if (hasSelectedPart && !song.measures.empty())
        {
            if (summary.placementOffsetTicks == 0)
            {
                document.measures().clear();
            }
            for (const ImportedMeasure &source : song.measures)
            {
                document.measures().push_back(convertMeasure(
                    source, song.ticksPerBeat, document.ticksPerBeat(), summary.placementOffsetTicks));
            }
            std::sort(document.measures().begin(), document.measures().end(),
                      [](const TrackChartMeasure &left, const TrackChartMeasure &right)
                      { return left.startTick < right.startTick; });
            if (summary.placementOffsetTicks == 0)
            {
                document.setBeatsPerMeasure(song.measures.front().numerator);
            }
        }

        for (const SelectedImportPart &selection : selections)
        {
            if (!selection.enabled || selection.sourceIndex >= song.parts.size())
            {
                continue;
            }

            const ImportedPart &source = song.parts[selection.sourceIndex];
            TrackPart appliedPart;
            appliedPart.name = selection.name.empty() ? source.name : selection.name;
            appliedPart.stringCount = openchordix::track::clampTrackStringCount(selection.stringCount);
            appliedPart.tuning = openchordix::track::normalizeTrackTuning(
                selection.tuning, appliedPart.name, appliedPart.stringCount);
            summary.appliedParts.push_back(appliedPart);

            for (const ImportedNote &sourceNote : source.notes)
            {
                TrackTabNote note;
                note.part = appliedPart.name;
                note.tick = convertTick(sourceNote.tick, song.ticksPerBeat, document.ticksPerBeat()) +
                            summary.placementOffsetTicks;
                note.duration = std::max(1, convertTick(sourceNote.duration, song.ticksPerBeat, document.ticksPerBeat()));
                note.stringIndex = std::clamp(sourceNote.stringIndex.value_or(0), 0, appliedPart.stringCount - 1);
                note.fret = std::max(0, sourceNote.fret.value_or(0));
                note.noteType = sourceNote.technique.noteType;
                note.slideType = sourceNote.technique.slideType;
                note.harmonicType = sourceNote.technique.harmonicType;
                note.pluckStyle = sourceNote.technique.pluckStyle;
                note.hammerOn = sourceNote.technique.hammerOn;
                note.pullOff = sourceNote.technique.pullOff;
                note.bend = sourceNote.technique.bend;
                note.vibrato = sourceNote.technique.vibrato;
                note.palmMute = sourceNote.technique.palmMute;
                note.letRing = sourceNote.technique.letRing;
                note.staccato = sourceNote.technique.staccato;
                note.tremoloPicking = sourceNote.technique.tremoloPicking;
                note.trill = sourceNote.technique.trill;
                note.accent = sourceNote.technique.accent;
                note.heavyAccent = sourceNote.technique.heavyAccent;
                document.notes().push_back(std::move(note));
                ++summary.addedNotes;
            }
        }

        std::sort(document.notes().begin(), document.notes().end(),
                  [](const TrackTabNote &left, const TrackTabNote &right)
                  {
                      if (left.part != right.part)
                      {
                          return left.part < right.part;
                      }
                      if (left.tick != right.tick)
                      {
                          return left.tick < right.tick;
                      }
                      return left.stringIndex < right.stringIndex;
                  });
        return summary;
    }
}
