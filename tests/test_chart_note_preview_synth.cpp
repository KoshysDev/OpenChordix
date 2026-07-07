#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "track/ChartNotePreviewSynth.h"

TEST_CASE("Chart note preview maps string fret to MIDI and frequency", "[track][preview]")
{
    using namespace openchordix::track;

    const std::vector<std::string> tuning = {"E4", "B3", "G3", "D3", "A2", "E2"};
    const auto highEThirdFret = chartNoteMidiForStringFret(tuning, 0, 3);
    REQUIRE(highEThirdFret.has_value());
    CHECK(*highEThirdFret == 67);
    CHECK(chartNoteFrequencyForMidi(69) == Catch::Approx(440.0));
    CHECK_FALSE(chartNoteMidiForStringFret(tuning, 9, 0).has_value());
}

TEST_CASE("Chart note preview schedules through tempo map and offset", "[track][preview]")
{
    using namespace openchordix::track;

    const TempoMap map(960, {TempoEvent{0, 120.0}, TempoEvent{1920, 60.0}});
    const std::vector<ChartNotePreviewSourceNote> notes = {
        ChartNotePreviewSourceNote{.tick = 960, .durationTicks = 480, .stringIndex = 0, .fret = 0, .tuning = {"E4"}},
        ChartNotePreviewSourceNote{.tick = 2880, .durationTicks = 960, .stringIndex = 0, .fret = 12, .tuning = {"E4"}},
    };

    const auto noOffset = buildChartNotePreviewEvents(notes, map, 48000, 0, 0.5f);
    REQUIRE(noOffset.size() == 2);
    CHECK(noOffset[0].frame == 24000);
    CHECK(noOffset[1].frame == 96000);
    CHECK(noOffset[0].midiNote == 64);
    CHECK(noOffset[1].midiNote == 76);

    const auto delayed = buildChartNotePreviewEvents(notes, map, 48000, 100, 0.5f);
    REQUIRE(delayed.size() == 2);
    CHECK(delayed[0].frame == 28800);

    const auto mutedByVolume = buildChartNotePreviewEvents(notes, map, 48000, 0, 0.0f);
    CHECK(mutedByVolume.empty());
}

TEST_CASE("Chart note preview schedules chords at the same frame", "[track][preview]")
{
    using namespace openchordix::track;

    const TempoMap map(960, {TempoEvent{0, 120.0}});
    const std::vector<ChartNotePreviewSourceNote> chord = {
        ChartNotePreviewSourceNote{.tick = 960, .durationTicks = 960, .stringIndex = 0, .fret = 0, .tuning = {"E4", "B3"}},
        ChartNotePreviewSourceNote{.tick = 960, .durationTicks = 960, .stringIndex = 1, .fret = 1, .tuning = {"E4", "B3"}},
    };

    const auto events = buildChartNotePreviewEvents(chord, map, 44100, 0, 0.4f);
    REQUIRE(events.size() == 2);
    CHECK(events[0].frame == events[1].frame);
    CHECK(events[0].durationFrames > 0);
    CHECK(events[1].durationFrames > 0);
}
