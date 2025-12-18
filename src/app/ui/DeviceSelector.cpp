#include "ui/DeviceSelector.h"

#include <sstream>

namespace
{
    bool isDefault(const RtAudio::DeviceInfo &info, DeviceRole role)
    {
        return role == DeviceRole::Input ? info.isDefaultInput : info.isDefaultOutput;
    }

    // helpers now declared in header
}

int channelCount(const RtAudio::DeviceInfo &info, DeviceRole role)
{
    return role == DeviceRole::Input ? info.inputChannels : info.outputChannels;
}

bool isUsable(const RtAudio::DeviceInfo &info, DeviceRole role)
{
    return channelCount(info, role) > 0;
}

std::vector<DeviceOption> DeviceSelector::makeOptions(const std::vector<DeviceEntry> &entries,
                                                      DeviceRole role,
                                                      std::optional<unsigned int> selectedId)
{
    std::vector<DeviceOption> options;
    options.reserve(entries.size());
    for (const auto &entry : entries)
    {
        DeviceOption opt{};
        opt.id = entry.id;
        opt.info = entry.info;
        opt.selected = selectedId.has_value() && *selectedId == entry.id;
        opt.disabled = !isUsable(entry.info, role);
        options.push_back(opt);
    }
    return options;
}

bool DeviceSelector::combo(const char *label,
                           const std::vector<DeviceOption> &options,
                           DeviceRole role,
                           const std::function<void(unsigned int)> &onSelect)
{
    std::string currentLabel = "No devices";
    for (const auto &opt : options)
    {
        if (opt.selected)
        {
            currentLabel = opt.info.name;
            if (isDefault(opt.info, role))
            {
                currentLabel += " (default)";
            }
            break;
        }
    }

    bool changed = false;
    if (ImGui::BeginCombo(label, currentLabel.c_str()))
    {
        for (const auto &opt : options)
        {
            if (opt.disabled)
            {
                continue;
            }
            bool selected = opt.selected;
            std::string labelStr = opt.info.name;
            if (isDefault(opt.info, role))
            {
                labelStr += " (default)";
            }
            if (ImGui::Selectable(labelStr.c_str(), selected))
            {
                onSelect(opt.id);
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

bool DeviceSelector::list(const char *id,
                          const std::vector<DeviceOption> &options,
                          DeviceRole role,
                          const std::function<void(unsigned int)> &onSelect,
                          ImVec2 size)
{
    bool changed = false;
    if (options.empty())
    {
        ImGui::TextDisabled("No devices reported by this API.");
        return false;
    }

    ImGui::BeginChild(id, size, true);
    for (const auto &opt : options)
    {
        ImGui::BeginDisabled(opt.disabled);
        std::ostringstream row;
        row << "#" << opt.id << "  " << opt.info.name;
        if (isDefault(opt.info, role))
        {
            row << "  (default)";
        }
        if (ImGui::Selectable(row.str().c_str(), opt.selected))
        {
            onSelect(opt.id);
            changed = true;
        }
        ImGui::SameLine();
        if (role == DeviceRole::Input)
        {
            ImGui::TextDisabled("%uch in / %uch out", opt.info.inputChannels, opt.info.outputChannels);
        }
        else
        {
            ImGui::TextDisabled("%uch out", opt.info.outputChannels);
        }
        ImGui::EndDisabled();
    }
    ImGui::EndChild();
    return changed;
}
