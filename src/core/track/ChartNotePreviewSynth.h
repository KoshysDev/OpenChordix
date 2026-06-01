#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "track/TempoMap.h"

namespace openchordix::track
{
    struct ChartNotePreviewSourceNote
    {
        int tick = 0;
        int durationTicks = 1;
        int stringIndex = 0;
        int fret = 0;
        std::vector<std::string> tuning;
    };

    struct ChartNotePreviewEvent
    {
        std::size_t frame = 0;
        int midiNote = -1;
        double frequencyHz = 0.0;
        std::size_t durationFrames = 0;
        float amplitude = 0.0f;
    };

    std::optional<int> chartNoteMidiForStringFret(const std::vector<std::string> &tuning,
                                                  int stringIndex,
                                                  int fret);
    double chartNoteFrequencyForMidi(int midiNote);
    std::vector<ChartNotePreviewEvent> buildChartNotePreviewEvents(const std::vector<ChartNotePreviewSourceNote> &notes,
                                                                   const TempoMap &tempoMap,
                                                                   unsigned int sampleRate,
                                                                   int chartAudioOffsetMs,
                                                                   float amplitude);
}
