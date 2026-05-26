#pragma once

#include <atomic>
#include <utility>
#include <vector>

#include <rtaudio/RtAudio.h>

class NoteConverter;

struct ConsoleFlowEntryTestState
{
    bool constructed = false;
    bool runCalled = false;
    int runResult = 0;
    std::vector<RtAudio::Api> apis{};
};

class ConsoleFlow
{
public:
    ConsoleFlow(std::vector<RtAudio::Api> apis, NoteConverter &)
    {
        auto &testState = state();
        testState.constructed = true;
        testState.apis = std::move(apis);
    }

    static ConsoleFlowEntryTestState &state()
    {
        static ConsoleFlowEntryTestState testState;
        return testState;
    }

    static void resetTestState()
    {
        state() = ConsoleFlowEntryTestState{};
    }

    int run(std::atomic<bool> &)
    {
        auto &testState = state();
        testState.runCalled = true;
        return testState.runResult;
    }
};
