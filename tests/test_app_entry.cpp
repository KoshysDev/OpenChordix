#include <catch2/catch_test_macros.hpp>

#include <ostream>
#include <sstream>
#include <string>

#include "GraphicsContext.h"
#include "GraphicsFlow.h"
#include "audio/AudioManager.h"
#include "console/ConsoleFlow.h"

#define main openchordix_app_main
#include "../src/app/main.cpp"
#undef main

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

void resetEntryTestState()
{
    AudioManager::resetTestState();
    GraphicsContext::resetTestState();
    GraphicsFlow::resetTestState();
    ConsoleFlow::resetTestState();
}
}

TEST_CASE("main returns an error when no audio APIs are available", "[app][entry]")
{
    resetEntryTestState();

    std::ostringstream errorOutput;
    ScopedStreamRedirect redirectError(std::cerr, errorOutput.rdbuf());
    char executable[] = "openchordix";
    char *argv[] = {executable};

    REQUIRE(openchordix_app_main(1, argv) == 1);
    CHECK(errorOutput.str().find("No usable RtAudio APIs found") != std::string::npos);
}

TEST_CASE("main forwards debug mode into the graphics flow", "[app][entry]")
{
    resetEntryTestState();
    AudioManager::state().availableApis = {RtAudio::Api::WINDOWS_DS};
    GraphicsFlow::state().runResult = 17;

    char executable[] = "openchordix";
    char debugFlag[] = "--debug";
    char *argv[] = {executable, debugFlag};

    REQUIRE(openchordix_app_main(2, argv) == 17);
    CHECK(GraphicsContext::state().windowTitle == "OpenChordix");
    CHECK(GraphicsFlow::state().constructed);
    CHECK(GraphicsFlow::state().runCalled);
    CHECK(GraphicsFlow::state().enableDevTools);
    CHECK(GraphicsFlow::state().apis == std::vector<RtAudio::Api>{RtAudio::Api::WINDOWS_DS});
}

TEST_CASE("main falls back to console mode when GUI bootstrap fails", "[app][entry]")
{
    resetEntryTestState();
    AudioManager::state().availableApis = {RtAudio::Api::UNIX_JACK};
    GraphicsContext::state().windowInitResult = false;
    ConsoleFlow::state().runResult = 9;

    std::ostringstream errorOutput;
    ScopedStreamRedirect redirectError(std::cerr, errorOutput.rdbuf());
    char executable[] = "openchordix";
    char *argv[] = {executable};

    REQUIRE(openchordix_app_main(1, argv) == 9);
    CHECK(ConsoleFlow::state().constructed);
    CHECK(ConsoleFlow::state().runCalled);
    CHECK(ConsoleFlow::state().apis == std::vector<RtAudio::Api>{RtAudio::Api::UNIX_JACK});
    CHECK(errorOutput.str().find("falling back to console mode") != std::string::npos);
}
