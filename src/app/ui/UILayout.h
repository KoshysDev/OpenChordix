#pragma once

#include <array>

#include <imgui/imgui.h>

struct PanelLayout
{
    bool windowOpen = false;
    ImVec2 contentSize{};
};

class UILayout
{
public:
    static PanelLayout beginFullscreen(const char *windowName,
                                       const char *childName,
                                       float margin,
                                       const std::array<ImVec4, 4> &bgColors,
                                       ImGuiWindowFlags extraFlags = 0);
    static void endFullscreen();
};
