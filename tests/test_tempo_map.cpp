#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

#include "track/TempoMap.h"

TEST_CASE("TempoMap converts single tempo ticks and seconds", "[track][tempo]")
{
    using openchordix::track::TempoEvent;
    using openchordix::track::TempoMap;

    const TempoMap map(960, {TempoEvent{0, 120.0}});
    CHECK(map.validate());
    CHECK(map.tickToSeconds(0) == Catch::Approx(0.0));
    CHECK(map.tickToSeconds(960) == Catch::Approx(0.5));
    CHECK(map.tickToSeconds(1920) == Catch::Approx(1.0));
    CHECK(map.secondsToTick(0.0) == 0);
    CHECK(map.secondsToTick(0.5) == 960);
    CHECK(map.secondsToTick(1.0) == 1920);

    const TempoMap slow(960, {TempoEvent{0, 60.0}});
    CHECK(slow.tickToSeconds(960) == Catch::Approx(1.0));
}

TEST_CASE("TempoMap converts across multiple tempo changes", "[track][tempo]")
{
    using openchordix::track::TempoEvent;
    using openchordix::track::TempoMap;

    const TempoMap map(960, {
                                  TempoEvent{0, 120.0},
                                  TempoEvent{1920, 60.0},
                              });
    CHECK(map.tickToSeconds(1920) == Catch::Approx(1.0));
    CHECK(map.tickToSeconds(2880) == Catch::Approx(2.0));
    CHECK(map.secondsToTick(1.0) == 1920);
    CHECK(map.secondsToTick(2.0) == 2880);
    CHECK(map.bpmAtTick(1919) == Catch::Approx(120.0));
    CHECK(map.bpmAtTick(1920) == Catch::Approx(60.0));
    REQUIRE(map.nextEventAfterTick(0).has_value());
    CHECK(map.nextEventAfterTick(0)->tick == 1920);
}

TEST_CASE("TempoMap normalizes duplicates unsorted events and invalid BPM", "[track][tempo]")
{
    using openchordix::track::TempoEvent;
    using openchordix::track::TempoMap;

    const TempoMap map(960, {
                                  TempoEvent{2880, 90.0},
                                  TempoEvent{0, 120.0},
                                  TempoEvent{1920, -1.0},
                                  TempoEvent{2880, 60.0},
                              });
    REQUIRE(map.validate());
    REQUIRE(map.events().size() == 2);
    CHECK(map.events()[0].tick == 0);
    CHECK(map.events()[1].tick == 2880);
    CHECK(map.events()[1].bpm == Catch::Approx(60.0));

    TempoMap rejected(960);
    CHECK_FALSE(rejected.addEvent({480, 0.0}));
}

TEST_CASE("TempoMap round trips within one tick", "[track][tempo]")
{
    using openchordix::track::TempoEvent;
    using openchordix::track::TempoMap;

    const TempoMap map(960, {
                                  TempoEvent{0, 137.0},
                                  TempoEvent{1217, 83.0},
                                  TempoEvent{4096, 164.0},
                              });
    for (const int tick : {0, 1, 480, 960, 1216, 1217, 2000, 4096, 7777})
    {
        const int roundTrip = map.secondsToTick(map.tickToSeconds(tick));
        CHECK(std::abs(roundTrip - tick) <= 1);
    }
}
