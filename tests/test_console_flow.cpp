#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <ios>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "audio/AudioManager.h"

#include "../src/core/NoteConverter.cpp"
#include "../src/app/console/ConsolePrompter.cpp"
#include "../src/app/console/ConsoleFlow.cpp"

namespace
{
class ScopedStreamRedirect
{
public:
    ScopedStreamRedirect(std::ios &stream, std::streambuf *replacement)
        : stream_(stream),
          original_(stream.rdbuf(replacement))
    {
    }

    ~ScopedStreamRedirect()
    {
        stream_.rdbuf(original_);
    }

private:
    std::ios &stream_;
    std::streambuf *original_;
};

void seedInputOutputDevices(unsigned int inputId, unsigned int outputId)
{
    auto &state = AudioManager::state();
    state.defaultInputDeviceId = inputId;
    state.defaultOutputDeviceId = outputId;
    state.deviceInfos[inputId] = RtAudio::DeviceInfo{
        "Input Device",
        2,
        0};
    state.deviceInfos[outputId] = RtAudio::DeviceInfo{
        "Output Device",
        0,
        2};
}
}

TEST_CASE("ConsoleFlow fails fast when no APIs are available", "[console]")
{
    AudioManager::resetTestState();

    NoteConverter converter;
    ConsoleFlow flow({}, converter);
    std::atomic<bool> quitFlag{false};
    std::ostringstream errorOutput;
    ScopedStreamRedirect redirectError(std::cerr, errorOutput.rdbuf());

    CHECK(flow.run(quitFlag) == 1);
    CHECK(errorOutput.str().find("No RtAudio APIs compiled or found") != std::string::npos);
}

TEST_CASE("ConsolePrompter retries invalid input and accepts defaults", "[console]")
{
    AudioManager::resetTestState();
    seedInputOutputDevices(11, 22);

    ConsolePrompter prompter({RtAudio::Api::WINDOWS_DS});
    AudioManager manager(RtAudio::Api::WINDOWS_DS);

    std::istringstream input("0\n1\nnot-a-number\n\n\n");
    std::ostringstream standardOutput;
    std::ostringstream errorOutput;
    ScopedStreamRedirect redirectInput(std::cin, input.rdbuf());
    ScopedStreamRedirect redirectOutput(std::cout, standardOutput.rdbuf());
    ScopedStreamRedirect redirectError(std::cerr, errorOutput.rdbuf());

    CHECK(prompter.chooseApi() == RtAudio::Api::WINDOWS_DS);
    CHECK(prompter.chooseDevice(manager, DeviceRole::Input, 11) == 11);
    CHECK(prompter.chooseDevice(manager, DeviceRole::Output, 22) == 22);

    CHECK(errorOutput.str().find("Invalid input") != std::string::npos);
    CHECK(standardOutput.str().find("Selected INPUT device: Input Device") != std::string::npos);
    CHECK(standardOutput.str().find("Selected OUTPUT device: Output Device") != std::string::npos);
}

TEST_CASE("ConsoleFlow opens monitoring with standard buffer size for non-JACK APIs", "[console]")
{
    AudioManager::resetTestState();
    seedInputOutputDevices(3, 4);

    NoteConverter converter;
    ConsoleFlow flow({RtAudio::Api::WINDOWS_DS}, converter);
    std::atomic<bool> quitFlag{true};
    std::istringstream input("1\n\n\n");
    std::ostringstream standardOutput;
    ScopedStreamRedirect redirectInput(std::cin, input.rdbuf());
    ScopedStreamRedirect redirectOutput(std::cout, standardOutput.rdbuf());

    REQUIRE(flow.run(quitFlag) == 0);
    CHECK(AudioManager::state().constructedApis == std::vector<RtAudio::Api>{RtAudio::Api::WINDOWS_DS});
    CHECK(AudioManager::state().openedInputDeviceId == 3);
    CHECK(AudioManager::state().openedOutputDeviceId == 4);
    CHECK(AudioManager::state().openedSampleRate == 48000);
    CHECK(AudioManager::state().openedBufferFrames == 1024);
    CHECK(AudioManager::state().stopCalled);
    CHECK(AudioManager::state().closeCalled);
    CHECK(standardOutput.str().find("Pitch Detection Started") != std::string::npos);
}

TEST_CASE("ConsoleFlow lets JACK choose the buffer size automatically", "[console]")
{
    AudioManager::resetTestState();
    seedInputOutputDevices(7, 8);

    NoteConverter converter;
    ConsoleFlow flow({RtAudio::Api::UNIX_JACK}, converter);
    std::atomic<bool> quitFlag{true};
    std::istringstream input("1\n\n\n");
    ScopedStreamRedirect redirectInput(std::cin, input.rdbuf());

    REQUIRE(flow.run(quitFlag) == 0);
    CHECK(AudioManager::state().openedBufferFrames == 0);
}
