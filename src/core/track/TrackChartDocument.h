#pragma once

#include <filesystem>
#include <string>
#include <vector>

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
    int timelineEndTick() const;

    int ticksPerBeat() const { return ticksPerBeat_; }
    void setTicksPerBeat(int value);

    int beatsPerMeasure() const { return beatsPerMeasure_; }
    void setBeatsPerMeasure(int value);

    double previewStartSeconds() const { return previewStartSeconds_; }
    void setPreviewStartSeconds(double value);

    const std::string &lastError() const { return lastError_; }

private:
    std::vector<TrackTabNote> notes_;
    std::vector<TrackChartMeasure> measures_;
    int ticksPerBeat_ = kDefaultTicksPerBeat;
    int beatsPerMeasure_ = kDefaultBeatsPerMeasure;
    double previewStartSeconds_ = 0.0;
    mutable std::string lastError_;
};
