#pragma once

#include <filesystem>
#include <cstdint>
#include <string>
#include <vector>

#include "track/TempoMap.h"
#include "track/TrackTypes.h"

struct TrackTabNote
{
    std::string part;
    int tick = 0;
    int duration = 48;
    int stringIndex = 0;
    int fret = 0;
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
    std::uint64_t editorId = 0;
};

struct TrackChartMeasure
{
    int number = 0;
    int numerator = 4;
    int denominator = 4;
    int startTick = 0;
    int durationTicks = 0;
    bool pickup = false;
};

class TrackChartDocument
{
public:
    static constexpr int kDefaultTicksPerBeat = 48;
    static constexpr int kDefaultBeatsPerMeasure = 4;

    bool load(const std::filesystem::path &chartPath);
    bool save(const std::filesystem::path &chartPath, const TrackInfo &track) const;

    void clear();

    std::vector<TrackTabNote> &notes() { return notes_; }
    const std::vector<TrackTabNote> &notes() const { return notes_; }
    std::vector<TrackChartMeasure> &measures() { return measures_; }
    const std::vector<TrackChartMeasure> &measures() const { return measures_; }
    std::vector<openchordix::track::TempoEvent> &tempoEvents() { return tempoEvents_; }
    const std::vector<openchordix::track::TempoEvent> &tempoEvents() const { return tempoEvents_; }
    void setTempoEvents(std::vector<openchordix::track::TempoEvent> events, double fallbackBpm = openchordix::track::TempoMap::kDefaultBpm);
    openchordix::track::TempoMap tempoMap(double fallbackBpm = openchordix::track::TempoMap::kDefaultBpm) const;
    int timelineEndTick() const;

    int ticksPerBeat() const { return ticksPerBeat_; }
    void setTicksPerBeat(int value);

    int beatsPerMeasure() const { return beatsPerMeasure_; }
    void setBeatsPerMeasure(int value);

    double previewStartSeconds() const { return previewStartSeconds_; }
    void setPreviewStartSeconds(double value);
    int chartAudioOffsetMs() const { return chartAudioOffsetMs_; }
    void setChartAudioOffsetMs(int value);

    const std::string &lastError() const { return lastError_; }

private:
    std::vector<TrackTabNote> notes_;
    std::vector<TrackChartMeasure> measures_;
    std::vector<openchordix::track::TempoEvent> tempoEvents_;
    int ticksPerBeat_ = kDefaultTicksPerBeat;
    int beatsPerMeasure_ = kDefaultBeatsPerMeasure;
    double previewStartSeconds_ = 0.0;
    int chartAudioOffsetMs_ = 0;
    mutable std::string lastError_;
};
