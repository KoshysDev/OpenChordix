#include "AnimatedUI.h"

#include <algorithm>
#include <cmath>

namespace
{
    ImU32 toColor(const ImVec4 &c)
    {
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    float smoothLerp(float current, float target, float dt)
    {
        const float k = 10.0f;
        return current + (target - current) * (1.0f - std::exp(-k * dt));
    }
}

void AnimatedUI::beginFrame(float deltaSeconds)
{
    dt_ = deltaSeconds;
}

float AnimatedUI::animate(ImGuiID id, bool hovered, bool active)
{
    const float target = active ? 1.0f : (hovered ? 0.6f : 0.0f);
    float &value = animState_[id];
    value = smoothLerp(value, target, dt_);
    return value;
}

bool AnimatedUI::button(const char *label, const ImVec2 &size)
{
    ImGui::PushID(label);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 finalSize = size;
    if (finalSize.x <= 0.0f)
    {
        finalSize.x = ImGui::GetContentRegionAvail().x;
    }
    if (finalSize.y <= 0.0f)
    {
        finalSize.y = 44.0f;
    }

    ImGui::InvisibleButton("btn", finalSize);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const bool clicked = ImGui::IsItemClicked();
    const float a = animate(ImGui::GetItemID(), hovered, active);

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const float radius = 12.0f;
    const ImVec4 base = ImVec4(0.10f, 0.14f, 0.20f, 0.9f);
    const ImVec4 glow = ImVec4(0.24f, 0.56f, 0.92f, 0.25f * a);
    const ImVec4 fill = ImVec4(
        base.x + 0.25f * a,
        base.y + 0.28f * a,
        base.z + 0.32f * a,
        1.0f);

    dl->AddRectFilled(pos, ImVec2(pos.x + finalSize.x, pos.y + finalSize.y), toColor(fill), radius);
    if (a > 0.01f)
    {
        dl->AddRectFilled(ImVec2(pos.x - 3, pos.y - 3), ImVec2(pos.x + finalSize.x + 3, pos.y + finalSize.y + 3), toColor(glow), radius + 2.0f);
    }
    dl->AddRect(pos, ImVec2(pos.x + finalSize.x, pos.y + finalSize.y), toColor(ImVec4(0.35f, 0.58f, 0.92f, 0.4f * (0.3f + a))), radius, 0, 2.0f);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = ImVec2(
        pos.x + (finalSize.x - textSize.x) * 0.5f,
        pos.y + (finalSize.y - textSize.y) * 0.5f);
    dl->AddText(textPos, toColor(ImVec4(0.92f, 0.96f, 1.0f, 0.98f)), label);

    ImGui::PopID();
    return clicked;
}

bool AnimatedUI::sliderInt(const char *label, int *value, int min, int max)
{
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float height = 36.0f;
    const ImVec2 size(ImGui::GetContentRegionAvail().x, height);

    ImGui::InvisibleButton("slider", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const float anim = animate(ImGui::GetItemID(), hovered, active);

    bool changed = false;
    if (active)
    {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        float ratio = (mouse.x - pos.x) / size.x;
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        int newValue = static_cast<int>(std::round(std::lerp<float>(static_cast<float>(min), static_cast<float>(max), ratio)));
        if (newValue != *value)
        {
            *value = newValue;
            changed = true;
        }
    }

    const float t = std::clamp((*value - min) / static_cast<float>(std::max(1, max - min)), 0.0f, 1.0f);
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const float radius = 12.0f;
    ImVec4 base = ImVec4(0.11f, 0.13f, 0.18f, 1.0f);
    ImVec4 fill = ImVec4(0.22f, 0.56f, 0.96f, 1.0f);
    base.w = 0.9f;
    fill.x = std::clamp(fill.x + 0.1f * anim, 0.0f, 1.0f);
    fill.y = std::clamp(fill.y + 0.1f * anim, 0.0f, 1.0f);
    fill.z = std::clamp(fill.z + 0.1f * anim, 0.0f, 1.0f);

    const ImVec2 barMin = pos;
    const ImVec2 barMax = ImVec2(pos.x + size.x, pos.y + size.y);
    dl->AddRectFilled(barMin, barMax, toColor(base), radius);

    const ImVec2 fillMax = ImVec2(pos.x + size.x * t, pos.y + size.y);
    dl->AddRectFilled(barMin, fillMax, toColor(fill), radius);
    dl->AddRect(barMin, barMax, toColor(ImVec4(0.35f, 0.58f, 0.92f, 0.35f)), radius, 0, 2.0f);

    char valueText[32];
    std::snprintf(valueText, sizeof(valueText), "%d", *value);
    ImVec2 textSize = ImGui::CalcTextSize(valueText);
    ImVec2 textPos = ImVec2(barMax.x - textSize.x - 10.0f, pos.y + (size.y - textSize.y) * 0.5f);
    dl->AddText(textPos, toColor(ImVec4(0.95f, 0.98f, 1.0f, 1.0f)), valueText);

    ImGui::PopID();
    return changed;
}

void AnimatedUI::heading(const char *text, ImVec4 color)
{
    ImGui::Spacing();
    ImGui::TextColored(color, "%s", text);
    ImGui::Separator();
}
