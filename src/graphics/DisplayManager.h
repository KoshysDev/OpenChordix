#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <utility>
#include <vector>

namespace openchordix
{
    struct MonitorInfo
    {
        GLFWmonitor *handle = nullptr;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    struct MonitorList
    {
        std::vector<GLFWmonitor *> handles;
        std::vector<std::string> names;
    };

    struct VideoModeSet
    {
        std::vector<std::pair<int, int>> resolutions;
        std::vector<int> refreshRates;
    };

    class DisplayManager
    {
    public:
        static MonitorInfo bestMonitor();
        static MonitorInfo monitorForWindow(GLFWwindow *window);
        static MonitorList enumerateMonitors();
        static VideoModeSet modesForMonitor(GLFWmonitor *monitor);
        static GLFWvidmode currentVideoMode(GLFWmonitor *monitor);
    };
}
