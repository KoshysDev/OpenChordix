#include "DisplayManager.h"

#include <algorithm>
#include <set>

namespace openchordix
{
    namespace
    {
        MonitorInfo infoFor(GLFWmonitor *monitor)
        {
            MonitorInfo info{};
            if (!monitor)
            {
                return info;
            }

            int monitorX = 0;
            int monitorY = 0;
            int monitorW = 0;
            int monitorH = 0;
            glfwGetMonitorWorkarea(monitor, &monitorX, &monitorY, &monitorW, &monitorH);

            const GLFWvidmode *mode = glfwGetVideoMode(monitor);
            if ((!mode || monitorW <= 0 || monitorH <= 0) && mode)
            {
                monitorW = mode->width;
                monitorH = mode->height;
                glfwGetMonitorPos(monitor, &monitorX, &monitorY);
            }

            info.handle = monitor;
            info.x = monitorX;
            info.y = monitorY;
            info.width = monitorW;
            info.height = monitorH;
            return info;
        }
    } // namespace

    MonitorInfo DisplayManager::bestMonitor()
    {
        GLFWmonitor *primary = glfwGetPrimaryMonitor();
        if (primary)
        {
            return infoFor(primary);
        }

        GLFWmonitor *best = nullptr;
        int bestArea = 0;

        int count = 0;
        GLFWmonitor **monitors = glfwGetMonitors(&count);
        for (int i = 0; i < count; ++i)
        {
            GLFWmonitor *m = monitors[i];
            const GLFWvidmode *mode = glfwGetVideoMode(m);
            if (!mode)
            {
                continue;
            }
            const int area = mode->width * mode->height;
            if (!best || area > bestArea)
            {
                best = m;
                bestArea = area;
            }
        }

        return infoFor(best);
    }

    MonitorInfo DisplayManager::monitorForWindow(GLFWwindow *window)
    {
        if (!window)
        {
            return bestMonitor();
        }

        int wx = 0;
        int wy = 0;
        int ww = 0;
        int wh = 0;
        glfwGetWindowPos(window, &wx, &wy);
        glfwGetWindowSize(window, &ww, &wh);
        const int cx = wx + ww / 2;
        const int cy = wy + wh / 2;

        int count = 0;
        GLFWmonitor **monitors = glfwGetMonitors(&count);
        MonitorInfo bestCandidate{};
        int bestArea = 0;
        for (int i = 0; i < count; ++i)
        {
            GLFWmonitor *m = monitors[i];
            int mx = 0;
            int my = 0;
            int mw = 0;
            int mh = 0;
            glfwGetMonitorWorkarea(m, &mx, &my, &mw, &mh);
            const GLFWvidmode *mode = glfwGetVideoMode(m);
            if (!mode)
            {
                continue;
            }
            if (mw <= 0 || mh <= 0)
            {
                glfwGetMonitorPos(m, &mx, &my);
                mw = mode->width;
                mh = mode->height;
            }

            if (mw > 0 && mh > 0)
            {
                if (cx >= mx && cx < mx + mw && cy >= my && cy < my + mh)
                {
                    const int area = mw * mh;
                    if (area > bestArea)
                    {
                        bestArea = area;
                        bestCandidate = {m, mx, my, mw, mh};
                    }
                }
            }
        }

        if (bestCandidate.handle)
        {
            return bestCandidate;
        }

        return bestMonitor();
    }

    MonitorList DisplayManager::enumerateMonitors()
    {
        MonitorList list{};

        int monitorCount = 0;
        GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
        for (int i = 0; i < monitorCount; ++i)
        {
            list.handles.push_back(monitors[i]);
            const char *name = glfwGetMonitorName(monitors[i]);
            list.names.emplace_back(name ? name : "Unknown");
        }
        return list;
    }

    VideoModeSet DisplayManager::modesForMonitor(GLFWmonitor *monitor)
    {
        VideoModeSet modes{};
        if (!monitor)
        {
            return modes;
        }

        int count = 0;
        const GLFWvidmode *available = glfwGetVideoModes(monitor, &count);
        if (!available || count <= 0)
        {
            return modes;
        }

        std::set<std::pair<int, int>> uniqueRes;
        std::set<int> uniqueRefresh;

        for (int i = 0; i < count; ++i)
        {
            uniqueRes.emplace(available[i].width, available[i].height);
            uniqueRefresh.insert(available[i].refreshRate);
        }

        modes.resolutions.assign(uniqueRes.begin(), uniqueRes.end());
        modes.refreshRates.assign(uniqueRefresh.begin(), uniqueRefresh.end());
        std::sort(modes.resolutions.begin(), modes.resolutions.end());
        std::sort(modes.refreshRates.begin(), modes.refreshRates.end());
        return modes;
    }

    GLFWvidmode DisplayManager::currentVideoMode(GLFWmonitor *monitor)
    {
        GLFWvidmode mode{};
        const GLFWvidmode *current = monitor ? glfwGetVideoMode(monitor) : nullptr;
        if (current)
        {
            mode = *current;
        }
        return mode;
    }
}
