#pragma once

#include <string>

struct GraphicsContextTestState
{
    bool windowInitResult = true;
    bool rendererInitResult = true;
    std::string windowTitle{};
};

class GraphicsContext
{
public:
    static GraphicsContextTestState &state()
    {
        static GraphicsContextTestState testState;
        return testState;
    }

    static void resetTestState()
    {
        state() = GraphicsContextTestState{};
    }

    bool initializeWindowed(const char *title)
    {
        state().windowTitle = title == nullptr ? "" : title;
        return state().windowInitResult;
    }

    bool initializeRenderer()
    {
        return state().rendererInitResult;
    }
};
