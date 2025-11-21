#include "MainMenuScene.h"

#include <imgui/imgui.h>

void MainMenuScene::render(float /*dt*/, const FrameInput & /*input*/, GraphicsContext & /*gfx*/, std::atomic<bool> &quitFlag)
{
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("Main Menu", nullptr, flags))
    {
        ImGui::SetCursorPos(ImVec2(40.0f, 40.0f));
        ImGui::Text("Main Menu (placeholder)");
        if (ImGui::Button("Quit"))
        {
            quitFlag.store(true);
        }
    }
    ImGui::End();
}
