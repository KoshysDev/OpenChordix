#include "track/ChartNotePreviewSynth.h"

#include <algorithm>
#include <cmath>

#include "track/TuningLibrary.h"

namespace openchordix::track
{
    std::optional<int> chartNoteMidiForStringFret(const std::vector<std::string> &tuning,
                                                  int stringIndex,
                                                  int fret)
    {
        if (stringIndex < 0 || stringIndex >= static_cast<int>(tuning.size()) || fret < 0)
        {
            return std::nullopt;
        }
        const auto openString = tuningNoteMidi(tuning[static_cast<std::size_t>(stringIndex)]);
        if (!openString.has_value())
        {
            return std::nullopt;
        }
        const int midi = *openString + fret;
        if (midi < 0 || midi > 127)
        {
            return std::nullopt;
        }
        return midi;
    }

    double chartNoteFrequencyForMidi(int midiNote)
    {
        return midiNote < 0 || midiNote > 127 ? 0.0 : tuningFrequencyFromMidi(midiNote);
    }

    std::vector<ChartNotePreviewEvent> buildChartNotePreviewEvents(const std::vector<ChartNotePreviewSourceNote> &notes,
                                                                   const TempoMap &tempoMap,
                                                                   unsigned int sampleRate,
                                                                   int chartAudioOffsetMs,
                                                                   float amplitude)
    {
        std::vector<ChartNotePreviewEvent> events;
        if (sampleRate == 0 || amplitude <= 0.0f)
        {
            return events;
        }

        constexpr double kMinimumDurationSeconds = 0.08;
        constexpr double kMaximumDurationSeconds = 0.18;
        const double offsetSeconds = static_cast<double>(chartAudioOffsetMs) / 1000.0;
        events.reserve(notes.size());
        for (const ChartNotePreviewSourceNote &note : notes)
        {
            const auto midi = chartNoteMidiForStringFret(note.tuning, note.stringIndex, note.fret);
            if (!midi.has_value())
            {
                continue;
            }

            const double startSeconds = tempoMap.tickToSeconds(note.tick) + offsetSeconds;
            if (startSeconds < 0.0)
            {
                continue;
            }

            const double durationSeconds = std::clamp(
                tempoMap.durationSecondsForTickRange(note.tick, note.tick + std::max(1, note.durationTicks)),
                kMinimumDurationSeconds,
                kMaximumDurationSeconds);
            const auto durationFrames = static_cast<std::size_t>(
                std::max<long long>(1, std::llround(durationSeconds * static_cast<double>(sampleRate))));
            events.push_back({
                static_cast<std::size_t>(std::llround(startSeconds * static_cast<double>(sampleRate))),
                *midi,
                chartNoteFrequencyForMidi(*midi),
                durationFrames,
                amplitude,
            });
        }

        std::sort(events.begin(), events.end(),
                  [](const ChartNotePreviewEvent &left, const ChartNotePreviewEvent &right)
                  {
                      if (left.frame != right.frame)
                      {
                          return left.frame < right.frame;
                      }
                      return left.midiNote < right.midiNote;
                  });
        return events;
    }
}
