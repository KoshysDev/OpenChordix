#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <utility>
#include <vector>

#include "GraphicsContext.h"

struct DisplayState
{
    int monitorIndex = 0;
    int resolutionIndex = 0;
    int modeIndex = 0; // 0 fullscreen, 1 windowed, 2 borderless
    int refreshIndex = 0;
    bool monitorLocked = false;
    std::vector<GLFWmonitor *> monitors;
    std::vector<std::string> monitorNames;
    std::vector<std::pair<int, int>> resolutions;
    std::vector<int> refreshRates;
};

class DisplaySettingsController
{
public:
    DisplaySettingsController() = default;

    void refresh(GraphicsContext &gfx);
    void apply(GraphicsContext &gfx);
    void invalidateOptions() { needsRefresh_ = true; }
    void markDirty()
    {
        dirty_ = true;
        preserveSelection_ = true;
    }
    void clearDirty();

    DisplayState &state() { return state_; }
    const DisplayState &state() const { return state_; }
    bool dirty() const { return dirty_; }

    static const char *modeLabel(int idx);
    static std::string formatResolution(const std::pair<int, int> &res);

private:
    void clampIndices();
    void syncFromWindow(GraphicsContext &gfx);

    DisplayState state_{};
    bool preserveSelection_{false};
    bool needsRefresh_{true};
    bool dirty_{false};
};
