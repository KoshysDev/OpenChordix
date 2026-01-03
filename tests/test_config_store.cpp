#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "ConfigStore.h"
#include "audio/AudioConfig.h"

namespace {
std::filesystem::path configPath()
{
    ConfigStore store;
    return store.audioConfigPath();
}
}

TEST_CASE("ConfigStore saves and loads audio config", "[config]")
{
    std::filesystem::path path = configPath();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    ConfigStore store;
    AudioConfig config;
    config.api = RtAudio::Api::UNSPECIFIED;
    config.inputDeviceId = 1;
    config.outputDeviceId = 2;
    config.sampleRate = 44100;
    config.bufferFrames = 512;

    REQUIRE(store.saveAudioConfig(config));

    auto loaded = store.loadAudioConfig();
    REQUIRE(loaded.has_value());
    CHECK(loaded->api == config.api);
    CHECK(loaded->inputDeviceId == config.inputDeviceId);
    CHECK(loaded->outputDeviceId == config.outputDeviceId);
    CHECK(loaded->sampleRate == config.sampleRate);
    CHECK(loaded->bufferFrames == config.bufferFrames);

    std::filesystem::remove(path, ec);
}

TEST_CASE("ConfigStore rejects unusable configs", "[config]")
{
    std::filesystem::path path = configPath();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    ConfigStore store;
    AudioConfig config;
    config.inputDeviceId = 0;
    config.outputDeviceId = 0;

    REQUIRE_FALSE(store.saveAudioConfig(config));
    REQUIRE_FALSE(store.loadAudioConfig().has_value());
}

TEST_CASE("ConfigStore tolerates unknown keys and whitespace", "[config]")
{
    std::filesystem::path path = configPath();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    std::ofstream out(path, std::ios::trunc);
    REQUIRE(out.good());
    out << "api=1\n";
    out << "input_device=3\n";
    out << "output_device=4\n";
    out << "sample_rate=48000\n";
    out << "buffer_frames=512\n";
    out << "unknown_key=ignored\n";
    out.close();

    ConfigStore store;
    auto loaded = store.loadAudioConfig();
    REQUIRE(loaded.has_value());
    CHECK(loaded->inputDeviceId == 3);
    CHECK(loaded->outputDeviceId == 4);
    CHECK(loaded->sampleRate == 48000);
    CHECK(loaded->bufferFrames == 512);

    std::filesystem::remove(path, ec);
}
