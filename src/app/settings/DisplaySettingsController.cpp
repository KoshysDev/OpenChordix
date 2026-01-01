#include "settings/DisplaySettingsController.h"

#include <algorithm>
#include <iostream>

#include "DisplayManager.h"

using openchordix::DisplayManager;

const char *DisplaySettingsController::modeLabel(int idx)
{
    static const char *labels[] = {"Fullscreen", "Windowed", "Borderless fullscreen"};
    if (idx < 0 || idx >= 3)
    {
        return labels[0];
    }
    return labels[idx];
}

std::string DisplaySettingsController::formatResolution(const std::pair<int, int> &res)
{
    return std::to_string(res.first) + " x " + std::to_string(res.second);
}

void DisplaySettingsController::clampIndices()
{
    if (state_.monitors.empty())
    {
        state_.monitorIndex = 0;
        state_.resolutionIndex = 0;
        state_.refreshIndex = 0;
        return;
    }

    state_.monitorIndex = std::clamp(state_.monitorIndex, 0, static_cast<int>(state_.monitors.size()) - 1);
    if (state_.resolutions.empty())
    {
        state_.resolutionIndex = 0;
    }
    else
    {
        state_.resolutionIndex = std::clamp(state_.resolutionIndex, 0, static_cast<int>(state_.resolutions.size()) - 1);
    }

    if (state_.refreshRates.empty())
    {
        state_.refreshIndex = 0;
    }
    else
    {
        state_.refreshIndex = std::clamp(state_.refreshIndex, 0, static_cast<int>(state_.refreshRates.size()) - 1);
    }
}

void DisplaySettingsController::syncFromWindow(GraphicsContext &gfx)
{
    GLFWwindow *window = gfx.window();
    GLFWmonitor *windowMonitor = window ? glfwGetWindowMonitor(window) : nullptr;
    const auto currentMonitor = DisplayManager::monitorForWindow(window);

    auto list = DisplayManager::enumerateMonitors();
    state_.monitors = std::move(list.handles);
    state_.monitorNames = std::move(list.names);

    if (state_.monitors.empty())
    {
        return;
    }

    if (!preserveSelection_)
    {
        if (window && !windowMonitor)
        {
            int decorated = glfwGetWindowAttrib(window, GLFW_DECORATED);
            state_.modeIndex = decorated ? 1 : 2;
        }
        else if (windowMonitor)
        {
            state_.modeIndex = 0;
        }
    }

    if (state_.monitorIndex < 0 || state_.monitorIndex >= static_cast<int>(state_.monitors.size()))
    {
        state_.monitorIndex = 0;
        state_.monitorLocked = false;
    }

    if (!preserveSelection_ && currentMonitor.handle && !state_.monitorLocked)
    {
        for (size_t i = 0; i < state_.monitors.size(); ++i)
        {
            if (state_.monitors[i] == currentMonitor.handle)
            {
                state_.monitorIndex = static_cast<int>(i);
                break;
            }
        }
    }

    GLFWmonitor *monitor = state_.monitors[static_cast<size_t>(state_.monitorIndex)];
    const auto modeSet = DisplayManager::modesForMonitor(monitor);
    state_.resolutions = modeSet.resolutions;
    state_.refreshRates = modeSet.refreshRates;

    if (!preserveSelection_)
    {
        const GLFWvidmode mode = DisplayManager::currentVideoMode(monitor);
        auto resIt = std::find(state_.resolutions.begin(), state_.resolutions.end(), std::make_pair(mode.width, mode.height));
        if (resIt != state_.resolutions.end())
        {
            state_.resolutionIndex = static_cast<int>(std::distance(state_.resolutions.begin(), resIt));
        }
        auto refreshIt = std::find(state_.refreshRates.begin(), state_.refreshRates.end(), mode.refreshRate);
        if (refreshIt != state_.refreshRates.end())
        {
            state_.refreshIndex = static_cast<int>(std::distance(state_.refreshRates.begin(), refreshIt));
        }
    }
}

void DisplaySettingsController::refresh(GraphicsContext &gfx)
{
    if (needsRefresh_ || !preserveSelection_)
    {
        syncFromWindow(gfx);
        needsRefresh_ = false;
    }
    clampIndices();
}

void DisplaySettingsController::apply(GraphicsContext &gfx)
{
    preserveSelection_ = true;
    refresh(gfx);

    GLFWwindow *window = gfx.window();
    if (!window || state_.resolutions.empty())
    {
        clearDirty();
        return;
    }

    clampIndices();

    GLFWmonitor *targetMonitor = nullptr;
    if (state_.monitorIndex >= 0 && state_.monitorIndex < static_cast<int>(state_.monitors.size()))
    {
        targetMonitor = state_.monitors[static_cast<size_t>(state_.monitorIndex)];
    }
    if (!targetMonitor)
    {
        targetMonitor = DisplayManager::monitorForWindow(window).handle;
    }
    if (!targetMonitor)
    {
        targetMonitor = DisplayManager::bestMonitor().handle;
    }

    const GLFWvidmode targetMode = DisplayManager::currentVideoMode(targetMonitor);
    std::pair<int, int> res = targetMode.width > 0 && targetMode.height > 0 ? std::make_pair(targetMode.width, targetMode.height)
                                                                            : state_.resolutions[static_cast<size_t>(state_.resolutionIndex)];
    int refresh = targetMode.refreshRate > 0    ? targetMode.refreshRate
                  : state_.refreshRates.empty() ? 0
                                                : state_.refreshRates[static_cast<size_t>(state_.refreshIndex)];

    if (state_.modeIndex == 1)
    {
        res = state_.resolutions[static_cast<size_t>(state_.resolutionIndex)];
        refresh = 0;
    }

    int monitorX = 0;
    int monitorY = 0;
    glfwGetMonitorPos(targetMonitor, &monitorX, &monitorY);

    int areaX = monitorX;
    int areaY = monitorY;
    int areaW = targetMode.width;
    int areaH = targetMode.height;
    int workX = 0;
    int workY = 0;
    int workW = 0;
    int workH = 0;
    glfwGetMonitorWorkarea(targetMonitor, &workX, &workY, &workW, &workH);
    if (workW > 0 && workH > 0)
    {
        areaX = workX;
        areaY = workY;
        areaW = workW;
        areaH = workH;
    }

    int xPos = areaX + (areaW - res.first) / 2;
    int yPos = areaY + (areaH - res.second) / 2;

    if (state_.modeIndex == 0)
    {
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(window, targetMonitor, 0, 0, res.first, res.second, refresh);
    }
    else if (state_.modeIndex == 2)
    {
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
        int targetW = workW > 0 ? workW : res.first;
        int targetH = workH > 0 ? workH : res.second;
        glfwSetWindowMonitor(window, nullptr, areaX, areaY, targetW, targetH, 0);
        res.first = targetW;
        res.second = targetH;
    }
    else
    {
        glfwSetWindowMonitor(window, nullptr, xPos, yPos, res.first, res.second, 0);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
    }

    gfx.applyResize(static_cast<uint32_t>(res.first), static_cast<uint32_t>(res.second));

    int winX = 0;
    int winY = 0;
    int winW = 0;
    int winH = 0;
    int fbW = 0;
    int fbH = 0;
    glfwGetWindowPos(window, &winX, &winY);
    glfwGetWindowSize(window, &winW, &winH);
    glfwGetFramebufferSize(window, &fbW, &fbH);
    const char *monitorName = targetMonitor ? glfwGetMonitorName(targetMonitor) : "None";
    std::cout << "[Display] Mode " << state_.modeIndex << " monitor " << monitorName
              << " res " << res.first << "x" << res.second << " win " << winW << "x" << winH
              << " fb " << fbW << "x" << fbH << " pos (" << winX << "," << winY << ")" << std::endl;

    clearDirty();
}

void DisplaySettingsController::clearDirty()
{
    dirty_ = false;
    preserveSelection_ = false;
    needsRefresh_ = true;
}
