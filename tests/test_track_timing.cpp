#include <catch2/catch_test_macros.hpp>

#include "track/TrackTiming.h"

TEST_CASE("Measure length helper uses integer time signature math", "[track][timing]")
{
    using openchordix::track::measureLengthTicks;

    CHECK(measureLengthTicks(960, 4, 4).value() == 3840);
    CHECK(measureLengthTicks(960, 3, 4).value() == 2880);
    CHECK(measureLengthTicks(960, 6, 8).value() == 2880);
    CHECK(measureLengthTicks(960, 12, 8).value() == 5760);
    CHECK(measureLengthTicks(960, 15, 8).value() == 7200);
    CHECK(measureLengthTicks(960, 1, 16).value() == 240);
    CHECK(measureLengthTicks(960, 5, 16).value() == 1200);
    CHECK_FALSE(measureLengthTicks(960, 5, 10).has_value());
    CHECK_FALSE(measureLengthTicks(960, 0, 4).has_value());
}
