#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "track/TrackChartDocument.h"

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
    CHECK(reloaded.timelineEndTick() == 192);

    std::filesystem::remove(path, ec);
}
