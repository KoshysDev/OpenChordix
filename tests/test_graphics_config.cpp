#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "settings/GraphicsConfig.h"

TEST_CASE("GraphicsConfig clamps values to allowed ranges", "[graphics]")
{
    GraphicsConfig config;
    config.quality = -5;
    config.gamma = 5.0f;
    config.laneContrast = 99;
    config.highwayGlow = -1;
    config.hitFeedback = 10;

    config.clamp();

    REQUIRE(config.quality == 0);
    REQUIRE(config.gamma == Catch::Approx(2.6f));
    REQUIRE(config.laneContrast == 3);
    REQUIRE(config.highwayGlow == 0);
    REQUIRE(config.hitFeedback == 3);
}

TEST_CASE("GraphicsConfig keeps values in range", "[graphics]")
{
    GraphicsConfig config;
    config.quality = 2;
    config.gamma = 2.2f;
    config.laneContrast = 1;
    config.highwayGlow = 2;
    config.hitFeedback = 1;

    config.clamp();

    REQUIRE(config.quality == 2);
    REQUIRE(config.gamma == Catch::Approx(2.2f));
    REQUIRE(config.laneContrast == 1);
    REQUIRE(config.highwayGlow == 2);
    REQUIRE(config.hitFeedback == 1);
}
