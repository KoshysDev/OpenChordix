#pragma once

#include <imgui/imgui.h>
#include <unordered_map>
#include <string>

class AnimatedUI
{
public:
    void beginFrame(float deltaSeconds);

    bool button(const char *label, const ImVec2 &size = ImVec2(0, 0));

    bool sliderInt(const char *label, int *value, int min, int max);

    void heading(const char *text, ImVec4 color);

private:
    float animate(ImGuiID id, bool hovered, bool active);

    float dt_ = 1.0f / 60.0f;
    std::unordered_map<ImGuiID, float> animState_{};
};
