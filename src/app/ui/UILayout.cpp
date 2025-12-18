#include "ui/UILayout.h"

PanelLayout UILayout::beginFullscreen(const char *windowName,
                                      const char *childName,
                                      float margin,
                                      const std::array<ImVec4, 4> &bgColors,
                                      ImGuiWindowFlags extraFlags)
{
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | extraFlags;

    PanelLayout layout{};
    layout.windowOpen = ImGui::Begin(windowName, nullptr, flags);
    if (layout.windowOpen)
    {
        ImDrawList *bg = ImGui::GetBackgroundDrawList();
        bg->AddRectFilledMultiColor(ImVec2(0, 0), screen,
                                    ImGui::ColorConvertFloat4ToU32(bgColors[0]),
                                    ImGui::ColorConvertFloat4ToU32(bgColors[1]),
                                    ImGui::ColorConvertFloat4ToU32(bgColors[2]),
                                    ImGui::ColorConvertFloat4ToU32(bgColors[3]));

        ImGui::SetCursorPos(ImVec2(margin, margin));
        layout.contentSize = ImVec2(screen.x - margin * 2.0f, screen.y - margin * 2.0f);
    }
    return layout;
}

void UILayout::endFullscreen()
{
    ImGui::End();
}
