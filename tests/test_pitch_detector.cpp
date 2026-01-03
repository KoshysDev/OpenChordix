#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

#include "PitchDetector.h"

TEST_CASE("PitchDetector rejects invalid constructor parameters", "[pitch]")
{
    REQUIRE_THROWS_AS(PitchDetector(0, 512, 48000), std::runtime_error);
    REQUIRE_THROWS_AS(PitchDetector(1024, 0, 48000), std::runtime_error);
    REQUIRE_THROWS_AS(PitchDetector(1024, 512, 0), std::runtime_error);
}

TEST_CASE("PitchDetector detects a steady sine wave", "[pitch]")
{
    const uint_t bufferSize = 1024;
    const uint_t hopSize = 1024;
    const uint_t sampleRate = 48000;
    const float frequency = 440.0f;

    PitchDetector detector(bufferSize, hopSize, sampleRate);

    std::vector<float> buffer(hopSize, 0.0f);
    double phase = 0.0;
    const double phaseInc = 2.0 * M_PI * frequency / static_cast<double>(sampleRate);

    for (int frame = 0; frame < 60; ++frame)
    {
        for (uint_t i = 0; i < hopSize; ++i)
        {
            buffer[i] = static_cast<float>(std::sin(phase));
            phase += phaseInc;
            if (phase > 2.0 * M_PI)
            {
                phase -= 2.0 * M_PI;
            }
        }
        detector.process(buffer.data(), hopSize, 1);
    }

    float detected = detector.getPitchHz();
    REQUIRE(detected > 0.0f);

    // Allow octave ambiguity and some jitter depending on backend/method.
    float normalized = detected;
    while (normalized > frequency * 1.5f)
    {
        normalized *= 0.5f;
    }
    while (normalized < frequency / 1.5f)
    {
        normalized *= 2.0f;
    }
    float cents = 1200.0f * std::log2(normalized / frequency);
    REQUIRE(std::fabs(cents) <= 250.0f);
}
