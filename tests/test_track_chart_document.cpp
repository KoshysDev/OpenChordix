#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "track/TrackChartDocument.h"
#include "track/TrackEditorMath.h"

TEST_CASE("TrackChartDocument loads and saves note timelines", "[track]")
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "openchordix-track-chart-test.ocx";
    std::error_code ec;
    std::filesystem::remove(path, ec);

    {
        std::ofstream out(path, std::ios::trunc);
        REQUIRE(out.good());
        out << R"({
  "song_id": "test-song",
  "title": "Test",
  "artist": "Artist",
  "source": "Source",
  "mapper": "Mapper",
  "bpm": 120,
  "length": "01:23",
  "audio_file": "song.ogg",
  "parts": ["Lead Guitar", "Bass"],
  "ticks_per_beat": 48,
  "beats_per_measure": 4,
  "events": [],
  "notes": [
    {
      "part": "Lead Guitar",
      "tick": 96,
      "duration": 24,
      "string": 1,
      "fret": 7,
      "note_type": "ghost",
      "slide_type": "shift",
      "hammer_on": true,
      "vibrato": true
    },
    { "part": "Bass", "tick": 0, "duration": 48, "string": 4, "fret": 3 }
  ]
})";
    }

    TrackChartDocument document;
    REQUIRE(document.load(path));
    REQUIRE(document.notes().size() == 2);
    CHECK(document.ticksPerBeat() == 48);
    CHECK(document.beatsPerMeasure() == 4);
    CHECK(document.chartAudioOffsetMs() == 0);
    REQUIRE(document.tempoEvents().size() == 1);
    CHECK(document.tempoEvents().front().tick == 0);
    CHECK(document.tempoEvents().front().bpm == 120.0);
    CHECK(document.notes()[0].part == "Bass");
    CHECK(document.notes()[1].fret == 7);
    CHECK(document.notes()[1].noteType == "ghost");
    CHECK(document.notes()[1].slideType == "shift");
    CHECK(document.notes()[1].hammerOn);
    CHECK(document.notes()[1].vibrato);

    document.notes().push_back(TrackTabNote{
        .part = "Lead Guitar",
        .tick = 144,
        .duration = 48,
        .stringIndex = 0,
        .fret = 12,
        .noteType = "tie",
        .harmonicType = "natural",
        .pluckStyle = "tap",
        .bend = true,
        .tremoloPicking = true,
    });
    document.measures() = {
        TrackChartMeasure{.number = 1, .numerator = 1, .denominator = 4, .startTick = 0, .durationTicks = 48, .pickup = true},
        TrackChartMeasure{.number = 2, .numerator = 6, .denominator = 8, .startTick = 48, .durationTicks = 144},
    };
    document.setTempoEvents({
        openchordix::track::TempoEvent{0, 140.0, "test"},
        openchordix::track::TempoEvent{96, 70.0, "test"},
    });
    document.setChartAudioOffsetMs(125);

    TrackInfo track;
    track.id = "test-song";
    track.title = "Updated";
    track.artist = "Artist";
    track.source = "Source";
    track.mapper = "Mapper";
    track.bpm = 140;
    track.length = "02:00";
    track.audioFile = "song.ogg";
    track.parts = {
        TrackPart{.name = "Lead Guitar", .stringCount = 7, .tuning = {"e", "B", "G", "D", "A", "E", "B"}},
        TrackPart{.name = "Bass", .stringCount = 5, .tuning = {"G", "D", "A", "E", "B"}}};

    REQUIRE(document.save(path, track));

    nlohmann::json saved;
    {
        std::ifstream in(path);
        REQUIRE(in.good());
        in >> saved;
    }
    REQUIRE(saved["parts"].is_array());
    REQUIRE(saved["parts"].size() == 2);
    CHECK(saved["parts"][0]["name"] == "Lead Guitar");
    CHECK(saved["parts"][0]["string_count"] == 7);
    CHECK(saved["parts"][0]["tuning"][6] == "B");
    CHECK(saved["parts"][1]["string_count"] == 5);
    CHECK(saved["parts"][1]["tuning"][4] == "B");
    REQUIRE(saved["measures"].is_array());
    REQUIRE(saved["measures"].size() == 2);
    CHECK(saved["measures"][0]["pickup"] == true);
    CHECK(saved["measures"][1]["numerator"] == 6);
    REQUIRE(saved["tempo_events"].is_array());
    REQUIRE(saved["tempo_events"].size() == 2);
    CHECK(saved["tempo_events"][1]["tick"] == 96);
    CHECK(saved["tempo_events"][1]["bpm"] == 70.0);
    CHECK(saved["chart_audio_offset_ms"] == 125);

    TrackChartDocument reloaded;
    REQUIRE(reloaded.load(path));
    REQUIRE(reloaded.notes().size() == 3);
    CHECK(reloaded.notes().back().fret == 12);
    CHECK(reloaded.notes().back().noteType == "tie");
    CHECK(reloaded.notes().back().harmonicType == "natural");
    CHECK(reloaded.notes().back().pluckStyle == "tap");
    CHECK(reloaded.notes().back().bend);
    CHECK(reloaded.notes().back().tremoloPicking);
    REQUIRE(reloaded.measures().size() == 2);
    CHECK(reloaded.measures().front().pickup);
    CHECK(reloaded.measures()[1].durationTicks == 144);
    REQUIRE(reloaded.tempoEvents().size() == 2);
    CHECK(reloaded.tempoMap().tickToSeconds(48) == Catch::Approx(60.0 / 140.0));
    CHECK(reloaded.tempoEvents()[1].tick == 96);
    CHECK(reloaded.chartAudioOffsetMs() == 125);
    CHECK(reloaded.timelineEndTick() == 192);

    std::filesystem::remove(path, ec);
}

TEST_CASE("Track chart audio offset changes conversion without mutating notes", "[track][timing]")
{
    using namespace openchordix::track;
    using namespace openchordix::track::editor;

    TrackChartDocument document;
    document.setTicksPerBeat(960);
    document.setTempoEvents({TempoEvent{0, 120.0}});
    document.notes().push_back(TrackTabNote{.part = "Lead", .tick = 960, .duration = 480, .stringIndex = 0, .fret = 3});

    const TempoMap map = document.tempoMap();
    CHECK(map.secondsToTick(chartSecondsFromAudioSeconds(0.5, document.chartAudioOffsetMs())) == 960);

    document.setChartAudioOffsetMs(100);
    CHECK(map.secondsToTick(chartSecondsFromAudioSeconds(0.5, document.chartAudioOffsetMs())) == 768);
    CHECK(document.notes().front().tick == 960);

    document.setChartAudioOffsetMs(-100);
    CHECK(map.secondsToTick(chartSecondsFromAudioSeconds(0.5, document.chartAudioOffsetMs())) == 1152);
    CHECK(document.notes().front().tick == 960);

    document.setPreviewStartSeconds(10.0);
    CHECK(document.chartAudioOffsetMs() == -100);
    CHECK(document.notes().front().tick == 960);
}
