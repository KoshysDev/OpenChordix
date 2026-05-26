#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "track/TrackEditorMath.h"

TEST_CASE("TrackEditor math formats and parses song clocks", "[track][editor]")
{
    using namespace openchordix::track::editor;

    CHECK(parseLengthSeconds("00:00") == 0);
    CHECK(parseLengthSeconds("01:40") == 100);
    CHECK(parseLengthSeconds("12:34") == 754);
    CHECK(parseLengthSeconds("bad") == 0);
    CHECK(parseLengthSeconds("12:xx") == 0);

    CHECK(formatClock(-2.0) == "00:00");
    CHECK(formatClock(100.1) == "01:40");
    CHECK(formatClock(754.4) == "12:34");
    CHECK(formatClock(754.6) == "12:35");
}

TEST_CASE("TrackEditor math quantizes ticks from snap settings", "[track][editor]")
{
    using namespace openchordix::track::editor;

    CHECK(snapTickSize(48, 0) == 48);
    CHECK(snapTickSize(48, 2) == 12);
    CHECK(snapTickSize(48, 99) == 6);

    CHECK(quantizeTick(-5, 48, 2) == 0);
    CHECK(quantizeTick(5, 48, 2) == 0);
    CHECK(quantizeTick(11, 48, 2) == 12);
    CHECK(quantizeTick(23, 48, 2) == 24);
}

TEST_CASE("TrackEditor math keeps timeline conversions stable", "[track][editor]")
{
    using namespace openchordix::track::editor;

    CHECK(timelineTotalBeats(90, 120, 4) == 180);
    CHECK(timelineTotalBeats(10, 120, 4) == 20);
    CHECK(timelineTotalTicks(90, 120, 4, 48) == 8640);

    CHECK(clampTransportCursor(-1.0, 120) == 0.0);
    CHECK(clampTransportCursor(140.0, 120) == 120.0);

    const double tick = timelineTickFromSeconds(30.0, 120, 48, 90);
    CHECK(tick == Catch::Approx(2880.0));
    CHECK(timelineSecondsFromTick(tick, 120, 48) == Catch::Approx(30.0));

    const double clampedTick = timelineTickFromSeconds(120.0, 120, 48, 90);
    CHECK(clampedTick == Catch::Approx(timelineTickFromSeconds(90.0, 120, 48, 90)));
}
