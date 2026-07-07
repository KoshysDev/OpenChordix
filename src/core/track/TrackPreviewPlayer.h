#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "track/TempoMap.h"

namespace openchordix::track
{
    struct TrackPreviewMixSettings
    {
        float songVolume = 1.0f;
        bool songMuted = false;
        float metronomeVolume = 0.65f;
        bool metronomeMuted = false;
        bool metronomeEnabled = false;
        float notePreviewVolume = 0.75f;
        bool notePreviewMuted = false;
        bool notePreviewEnabled = false;
    };

    struct TrackPreviewTimingMeasure
    {
        int startTick = 0;
        int durationTicks = 0;
        int numerator = 4;
        int denominator = 4;
    };

    struct TrackPreviewTimingNote
    {
        int tick = 0;
        int durationTicks = 0;
        int stringIndex = 0;
        int fret = 0;
        int midiNote = -1;
        double frequencyHz = 0.0;
    };

    class TrackPreviewPlayer
    {
    public:
        TrackPreviewPlayer();
        ~TrackPreviewPlayer();

        bool play(const std::filesystem::path &audioPath, double startSeconds = 0.0);
        void stop();
        void update();
        void setMixSettings(const TrackPreviewMixSettings &settings);
        TrackPreviewMixSettings mixSettings() const;
        void setTimingPreview(int ticksPerBeat,
                              std::vector<TempoEvent> tempoEvents,
                              std::vector<TrackPreviewTimingMeasure> measures,
                              std::vector<TrackPreviewTimingNote> notes,
                              int chartAudioOffsetMs = 0);

        bool isPlaying() const;
        double currentTimeSeconds() const;
        double durationSeconds() const;
        std::string status() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
