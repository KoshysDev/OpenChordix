#pragma once

#include <atomic>
#include <vector>

#include <rtaudio/RtAudio.h>

class AnimatedUI;
class AudioSession;
class ConfigStore;
class GraphicsContext;
class NoteConverter;

struct GraphicsFlowTestState
{
    bool constructed = false;
    bool runCalled = false;
    bool enableDevTools = false;
    int runResult = 0;
    std::vector<RtAudio::Api> apis{};
};

class GraphicsFlow
{
public:
    GraphicsFlow(GraphicsContext &,
                 AudioSession &,
                 ConfigStore &,
                 NoteConverter &,
                 AnimatedUI &,
                 const std::vector<RtAudio::Api> &apis,
                 bool enableDevTools)
    {
        auto &testState = state();
        testState.constructed = true;
        testState.enableDevTools = enableDevTools;
        testState.apis = apis;
    }

    static GraphicsFlowTestState &state()
    {
        static GraphicsFlowTestState testState;
        return testState;
    }

    static void resetTestState()
    {
        state() = GraphicsFlowTestState{};
    }

    int run(std::atomic<bool> &)
    {
        auto &testState = state();
        testState.runCalled = true;
        return testState.runResult;
    }
};
