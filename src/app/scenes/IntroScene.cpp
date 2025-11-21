#include "IntroScene.h"

#include <imgui/imgui.h>
#include <algorithm>

IntroScene::IntroScene(float durationSec) : duration_(durationSec) {}

void IntroScene::render(float dt, const FrameInput & /*input*/, GraphicsContext &gfx, std::atomic<bool> & /*quitFlag*/)
{
    if (finished_)
    {
        return;
    }

    elapsed_ += dt;
    const float t = std::min(elapsed_ / duration_, 1.0f);
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("Intro", nullptr, flags))
    {
        ImGui::SetCursorPos(ImVec2(screen.x * 0.5f - 120.0f, screen.y * 0.5f - 20.0f));
        ImVec4 color = ImVec4(0.90f, 0.95f, 1.0f, 0.2f + 0.8f * t);
        ImGui::TextColored(color, "OpenChordix");
    }
    ImGui::End();

    if (elapsed_ >= duration_)
    {
        finished_ = true;
    }
}
