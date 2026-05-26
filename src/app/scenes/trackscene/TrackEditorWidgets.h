#pragma once

#include <array>
#include <string>

#include <imgui/imgui.h>

#include "trackscene/TrackEditorShared.h"

namespace track_editor
{
    inline bool drawStringCountCombo(const char *label, int &value)
    {
        bool changed = false;
        const std::string selectedLabel = std::to_string(value);
        if (ImGui::BeginCombo(label, selectedLabel.c_str()))
        {
            for (int stringCount = 1; stringCount <= openchordix::track::kMaxTrackStrings; ++stringCount)
            {
                const bool selected = value == stringCount;
                const std::string option = std::to_string(stringCount);
                if (ImGui::Selectable(option.c_str(), selected))
                {
                    value = stringCount;
                    changed = true;
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    inline void drawToggleChip(const char *label, bool &value)
    {
        if (value)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.92f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(kAccent.x, kAccent.y, kAccent.z, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(kAccent.x * 0.82f, kAccent.y * 0.82f, kAccent.z * 0.82f, 1.0f));
        }
        if (ImGui::Button(label, ImVec2(86.0f, 28.0f)))
        {
            value = !value;
        }
        if (value)
        {
            ImGui::PopStyleColor(3);
        }
    }

    inline void drawStepperField(const char *label,
                                 const char *id,
                                 int &value,
                                 int minValue,
                                 int maxValue,
                                 int step,
                                 const char *suffix,
                                 const ImVec2 &pos,
                                 float fieldWidth)
    {
        ImGui::SetCursorPos(pos);
        ImGui::TextDisabled("%s", label);
        ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 18.0f));
        ImGui::PushID(id);
        if (ImGui::Button("-", ImVec2(32.0f, 32.0f)))
        {
            value = std::max(minValue, value - step);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(fieldWidth);
        ImGui::InputInt("##value", &value, 0, 0, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
        value = std::clamp(value, minValue, maxValue);
        ImGui::SameLine();
        if (ImGui::Button("+", ImVec2(32.0f, 32.0f)))
        {
            value = std::min(maxValue, value + step);
        }
        if (suffix != nullptr && suffix[0] != '\0')
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", suffix);
        }
        ImGui::PopID();
    }

    inline void drawStepperSlider(const char *label,
                                  const char *id,
                                  int &value,
                                  int minValue,
                                  int maxValue,
                                  int step,
                                  const char *format,
                                  const ImVec2 &pos,
                                  float sliderWidth)
    {
        ImGui::SetCursorPos(pos);
        ImGui::TextDisabled("%s", label);
        ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 18.0f));
        ImGui::PushID(id);
        if (ImGui::Button("<", ImVec2(32.0f, 32.0f)))
        {
            value = std::max(minValue, value - step);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::SliderInt("##slider", &value, minValue, maxValue, format);
        ImGui::SameLine();
        if (ImGui::Button(">", ImVec2(32.0f, 32.0f)))
        {
            value = std::min(maxValue, value + step);
        }
        ImGui::PopID();
    }

    template <size_t N>
    int comboIndexFromValue(const std::array<const char *, N> &labels, const std::string &value)
    {
        for (int i = 0; i < static_cast<int>(labels.size()); ++i)
        {
            if (value == labels[static_cast<size_t>(i)])
            {
                return i;
            }
        }
        return 0;
    }

    template <size_t N>
    void assignComboValue(std::string &target, const std::array<const char *, N> &labels, int index)
    {
        const int safeIndex = std::clamp(index, 0, static_cast<int>(labels.size()) - 1);
        const std::string value = labels[static_cast<size_t>(safeIndex)];
        target = value == "none" ? "" : value;
    }
}
