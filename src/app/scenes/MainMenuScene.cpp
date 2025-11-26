#include "MainMenuScene.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <cfloat>

#ifndef OPENCHORDIX_VERSION
#define OPENCHORDIX_VERSION "0.0"
#endif

#ifndef OPENCHORDIX_OS
#if defined(_WIN32)
#define OPENCHORDIX_OS "Windows"
#elif defined(__APPLE__)
#define OPENCHORDIX_OS "macOS"
#elif defined(__linux__)
#define OPENCHORDIX_OS "Linux"
#else
#define OPENCHORDIX_OS "Unknown OS"
#endif
#endif

void MainMenuScene::render(float /*dt*/, const FrameInput & /*input*/, GraphicsContext & /*gfx*/, std::atomic<bool> &quitFlag)
{
    static const char *popupMessage = nullptr;

    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("Main Menu", nullptr, flags))
    {
        ImDrawList *bg = ImGui::GetBackgroundDrawList();
        bg->AddRectFilledMultiColor(ImVec2(0, 0), screen,
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.06f, 0.09f, 1.0f)),
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.05f, 0.08f, 1.0f)),
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.07f, 0.09f, 0.12f, 1.0f)),
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.10f, 0.14f, 1.0f)));

        const float contentWidth = std::clamp(screen.x * 0.65f, 360.0f, 640.0f);
        const float buttonHeight = 64.0f;
        const float spacing = 18.0f;
        const float headerHeight = 60.0f;
        const float contentHeight = headerHeight + 4.0f * (buttonHeight + spacing) + spacing;
        ImVec2 start((screen.x - contentWidth) * 0.5f, (screen.y - contentHeight) * 0.5f);
        ImGui::SetCursorPos(start);
        ImGui::BeginGroup();

        ImGui::TextColored(ImVec4(0.76f, 0.86f, 1.0f, 1.0f), "OpenChordix");
        ImGui::TextDisabled("Welcome! Choose what you want to do.");
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        ImVec2 buttonSize(contentWidth, buttonHeight);
        if (ui_.button("Play", buttonSize))
        {
            popupMessage = "Play mode is not implemented yet.";
            ImGui::OpenPopup("feature_missing");
        }
        ImGui::Dummy(ImVec2(0.0f, spacing));
        if (ui_.button("Tuner", buttonSize))
        {
            pendingAction_ = Action::OpenTuner;
        }
        ImGui::Dummy(ImVec2(0.0f, spacing));
        if (ui_.button("Settings", buttonSize))
        {
            popupMessage = "Settings are not implemented yet.";
            ImGui::OpenPopup("feature_missing");
        }
        ImGui::Dummy(ImVec2(0.0f, spacing));
        if (ui_.button("Quit", buttonSize))
        {
            quitFlag.store(true);
        }

        ImGui::EndGroup();

        if (popupMessage)
        {
            ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, screen.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(480.0f, FLT_MAX));
        }
        if (ImGui::BeginPopup("feature_missing"))
        {
            ImGui::TextWrapped("%s", popupMessage);
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(200.0f, 48.0f)))
            {
                ImGui::CloseCurrentPopup();
                popupMessage = nullptr;
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(12.0f, screen.y - 12.0f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGuiWindowFlags infoFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("version_overlay", nullptr, infoFlags))
    {
        ImGui::Text("v%s \xe2\x80\xa2 %s", OPENCHORDIX_VERSION, OPENCHORDIX_OS);
    }
    ImGui::End();
}
