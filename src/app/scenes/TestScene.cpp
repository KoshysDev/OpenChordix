#include "TestScene.h"

#include <imgui/imgui.h>

TestScene::TestScene(AnimatedUI &ui)
    : ui_(ui)
{
}

void TestScene::render(float /*dt*/, const FrameInput & /*input*/, GraphicsContext & /*gfx*/, std::atomic<bool> & /*quitFlag*/)
{
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("Test Scene", nullptr, flags))
    {
        ImDrawList *bg = ImGui::GetBackgroundDrawList();
        bg->AddRectFilled(ImVec2(0, 0), screen, ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.05f, 0.07f, 1.0f)));

        const float padding = 12.0f;
        ImGui::SetCursorPos(ImVec2(padding, padding));
        if (ui_.button("Back to menu", ImVec2(180.0f, 44.0f)))
        {
            finished_ = true;
        }

        ImGui::SetCursorPos(ImVec2(screen.x * 0.5f - 120.0f, screen.y * 0.5f - 16.0f));
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 1.0f, 1.0f), "Test Scene");
    }
    ImGui::End();
}
