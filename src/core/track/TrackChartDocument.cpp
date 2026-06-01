#include "track/TrackChartDocument.h"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

namespace
{
    using json = nlohmann::json;

    TrackTabNote noteFromJson(const json &value, int defaultDuration)
    {
        TrackTabNote note;
        note.part = value.value("part", "");
        note.tick = std::max(0, value.value("tick", 0));
        note.duration = std::max(1, value.value("duration", defaultDuration));
        note.stringIndex = std::clamp(value.value("string", 0), 0, openchordix::track::kMaxTrackStrings - 1);
        note.fret = std::max(0, value.value("fret", 0));
        note.noteType = value.value("note_type", "normal");
        note.slideType = value.value("slide_type", "");
        note.harmonicType = value.value("harmonic_type", "");
        note.pluckStyle = value.value("pluck_style", "");
        note.hammerOn = value.value("hammer_on", false);
        note.pullOff = value.value("pull_off", false);
        note.bend = value.value("bend", false);
        note.vibrato = value.value("vibrato", false);
        note.palmMute = value.value("palm_mute", false);
        note.letRing = value.value("let_ring", false);
        note.staccato = value.value("staccato", false);
        note.tremoloPicking = value.value("tremolo_picking", false);
        note.trill = value.value("trill", false);
        note.accent = value.value("accent", false);
        note.heavyAccent = value.value("heavy_accent", false);
        return note;
    }

    json noteToJson(const TrackTabNote &note)
    {
        json value = json{
            {"part", note.part},
            {"tick", std::max(0, note.tick)},
            {"duration", std::max(1, note.duration)},
            {"string", std::clamp(note.stringIndex, 0, openchordix::track::kMaxTrackStrings - 1)},
            {"fret", std::max(0, note.fret)},
        };
        if (!note.noteType.empty() && note.noteType != "normal")
        {
            value["note_type"] = note.noteType;
        }
        if (!note.slideType.empty())
        {
            value["slide_type"] = note.slideType;
        }
        if (!note.harmonicType.empty())
        {
            value["harmonic_type"] = note.harmonicType;
        }
        if (!note.pluckStyle.empty())
        {
            value["pluck_style"] = note.pluckStyle;
        }
        if (note.hammerOn)
        {
            value["hammer_on"] = true;
        }
        if (note.pullOff)
        {
            value["pull_off"] = true;
        }
        if (note.bend)
        {
            value["bend"] = true;
        }
        if (note.vibrato)
        {
            value["vibrato"] = true;
        }
        if (note.palmMute)
        {
            value["palm_mute"] = true;
        }
        if (note.letRing)
        {
            value["let_ring"] = true;
        }
        if (note.staccato)
        {
            value["staccato"] = true;
        }
        if (note.tremoloPicking)
        {
            value["tremolo_picking"] = true;
        }
        if (note.trill)
        {
            value["trill"] = true;
        }
        if (note.accent)
        {
            value["accent"] = true;
        }
        if (note.heavyAccent)
        {
            value["heavy_accent"] = true;
        }
        return value;
    }

    TrackChartMeasure measureFromJson(const json &value)
    {
        TrackChartMeasure measure;
        measure.number = std::max(1, value.value("number", 1));
        measure.numerator = std::max(1, value.value("numerator", 4));
        measure.denominator = std::max(1, value.value("denominator", 4));
        measure.startTick = std::max(0, value.value("start_tick", 0));
        measure.durationTicks = std::max(1, value.value("duration_ticks", 1));
        measure.pickup = value.value("pickup", false);
        return measure;
    }

    json measureToJson(const TrackChartMeasure &measure)
    {
        return {
            {"number", measure.number},
            {"numerator", measure.numerator},
            {"denominator", measure.denominator},
            {"start_tick", measure.startTick},
            {"duration_ticks", measure.durationTicks},
            {"pickup", measure.pickup},
        };
    }

    openchordix::track::TempoEvent tempoEventFromJson(const json &value)
    {
        openchordix::track::TempoEvent event;
        event.tick = std::max(0, value.value("tick", 0));
        event.bpm = value.value("bpm", openchordix::track::TempoMap::kDefaultBpm);
        event.source = value.value("source", "");
        return event;
    }

    json tempoEventToJson(const openchordix::track::TempoEvent &event)
    {
        json value = {
            {"tick", std::max(0, event.tick)},
            {"bpm", event.bpm},
        };
        if (!event.source.empty())
        {
            value["source"] = event.source;
        }
        return value;
    }
}

bool TrackChartDocument::load(const std::filesystem::path &chartPath)
{
    clear();
    if (!std::filesystem::exists(chartPath))
    {
        return true;
    }

    std::ifstream in(chartPath);
    if (!in.good())
    {
        lastError_ = "Failed to open chart file.";
        return false;
    }

    json root;
    try
    {
        in >> root;
    }
    catch (const json::exception &)
    {
        lastError_ = "Chart file is not valid JSON.";
        return false;
    }

    if (!root.is_object())
    {
        lastError_ = "Chart root must be a JSON object.";
        return false;
    }

    setTicksPerBeat(root.value("ticks_per_beat", kDefaultTicksPerBeat));
    setBeatsPerMeasure(root.value("beats_per_measure", kDefaultBeatsPerMeasure));
    setPreviewStartSeconds(root.value("preview_start", 0.0));
    setChartAudioOffsetMs(root.value("chart_audio_offset_ms", 0));
    const double fallbackBpm = static_cast<double>(std::max(1, root.value("bpm", static_cast<int>(openchordix::track::TempoMap::kDefaultBpm))));

    std::vector<openchordix::track::TempoEvent> tempoEvents;
    if (const auto temposIt = root.find("tempo_events"); temposIt != root.end() && temposIt->is_array())
    {
        tempoEvents.reserve(temposIt->size());
        for (const auto &entry : *temposIt)
        {
            if (entry.is_object())
            {
                tempoEvents.push_back(tempoEventFromJson(entry));
            }
        }
    }
    if (tempoEvents.empty())
    {
        tempoEvents.push_back({0, fallbackBpm, "chart bpm"});
    }
    setTempoEvents(std::move(tempoEvents), fallbackBpm);

    if (const auto measuresIt = root.find("measures"); measuresIt != root.end() && measuresIt->is_array())
    {
        measures_.reserve(measuresIt->size());
        for (const auto &entry : *measuresIt)
        {
            if (entry.is_object())
            {
                measures_.push_back(measureFromJson(entry));
            }
        }
        std::sort(measures_.begin(), measures_.end(),
                  [](const TrackChartMeasure &left, const TrackChartMeasure &right)
                  { return left.startTick < right.startTick; });
    }

    if (const auto notesIt = root.find("notes"); notesIt != root.end() && notesIt->is_array())
    {
        notes_.reserve(notesIt->size());
        for (const auto &entry : *notesIt)
        {
            if (!entry.is_object())
            {
                continue;
            }
            notes_.push_back(noteFromJson(entry, ticksPerBeat_));
        }
    }

    std::sort(
        notes_.begin(),
        notes_.end(),
        [](const TrackTabNote &lhs, const TrackTabNote &rhs)
        {
            if (lhs.part != rhs.part)
            {
                return lhs.part < rhs.part;
            }
            if (lhs.tick != rhs.tick)
            {
                return lhs.tick < rhs.tick;
            }
            if (lhs.stringIndex != rhs.stringIndex)
            {
                return lhs.stringIndex < rhs.stringIndex;
            }
            return lhs.fret < rhs.fret;
        });

    return true;
}

bool TrackChartDocument::save(const std::filesystem::path &chartPath, const TrackInfo &track) const
{
    json root = json::object();
    if (std::filesystem::exists(chartPath))
    {
        std::ifstream in(chartPath);
        if (in.good())
        {
            try
            {
                in >> root;
            }
            catch (const json::exception &)
            {
                root = json::object();
            }
        }
    }

    if (!root.is_object())
    {
        root = json::object();
    }

    root["version"] = 1;
    root["song_id"] = track.id;
    root["title"] = track.title;
    root["artist"] = track.artist;
    root["source"] = track.source;
    root["mapper"] = track.mapper;
    root["bpm"] = track.bpm;
    root["length"] = track.length;
    root["audio_file"] = track.audioFile;
    root["parts"] = json::array();
    for (const TrackPart &part : track.parts)
    {
        root["parts"].push_back(json{
            {"name", part.name},
            {"string_count", openchordix::track::clampTrackStringCount(part.stringCount)},
            {"tuning", openchordix::track::normalizeTrackTuning(part.tuning, part.name, part.stringCount)},
        });
    }
    if (!root.contains("events") || !root["events"].is_array())
    {
        root["events"] = json::array();
    }
    root["ticks_per_beat"] = ticksPerBeat_;
    root["beats_per_measure"] = beatsPerMeasure_;
    root["tempo_events"] = json::array();
    const openchordix::track::TempoMap map = tempoMap(static_cast<double>(std::max(1, track.bpm)));
    for (const openchordix::track::TempoEvent &event : map.events())
    {
        root["tempo_events"].push_back(tempoEventToJson(event));
    }
    root["preview_start"] = previewStartSeconds_;
    root["chart_audio_offset_ms"] = chartAudioOffsetMs_;
    root["measures"] = json::array();
    for (const TrackChartMeasure &measure : measures_)
    {
        root["measures"].push_back(measureToJson(measure));
    }
    root["notes"] = json::array();
    for (const TrackTabNote &note : notes_)
    {
        root["notes"].push_back(noteToJson(note));
    }

    std::ofstream out(chartPath, std::ios::trunc);
    if (!out.good())
    {
        lastError_ = "Failed to open chart file for writing.";
        return false;
    }
    out << root.dump(2) << '\n';
    if (!out.good())
    {
        lastError_ = "Failed while writing chart file.";
        return false;
    }
    return true;
}

void TrackChartDocument::clear()
{
    notes_.clear();
    measures_.clear();
    tempoEvents_ = {{0, openchordix::track::TempoMap::kDefaultBpm, "default"}};
    ticksPerBeat_ = kDefaultTicksPerBeat;
    beatsPerMeasure_ = kDefaultBeatsPerMeasure;
    previewStartSeconds_ = 0.0;
    chartAudioOffsetMs_ = 0;
    lastError_.clear();
}

void TrackChartDocument::setTicksPerBeat(int value)
{
    ticksPerBeat_ = std::max(1, value);
}

void TrackChartDocument::setBeatsPerMeasure(int value)
{
    beatsPerMeasure_ = std::max(1, value);
}

void TrackChartDocument::setPreviewStartSeconds(double value)
{
    previewStartSeconds_ = std::max(0.0, value);
}

void TrackChartDocument::setChartAudioOffsetMs(int value)
{
    chartAudioOffsetMs_ = std::clamp(value, -600000, 600000);
}

void TrackChartDocument::setTempoEvents(std::vector<openchordix::track::TempoEvent> events, double fallbackBpm)
{
    openchordix::track::TempoMap map(ticksPerBeat_, std::move(events), fallbackBpm);
    tempoEvents_ = map.events();
}

openchordix::track::TempoMap TrackChartDocument::tempoMap(double fallbackBpm) const
{
    return openchordix::track::TempoMap(ticksPerBeat_, tempoEvents_, fallbackBpm);
}

int TrackChartDocument::timelineEndTick() const
{
    int endTick = 0;
    for (const TrackTabNote &note : notes_)
    {
        endTick = std::max(endTick, note.tick + note.duration);
    }
    for (const TrackChartMeasure &measure : measures_)
    {
        endTick = std::max(endTick, measure.startTick + measure.durationTicks);
    }
    return endTick;
}
