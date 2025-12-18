#pragma once

#include <imgui/imgui.h>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <rtaudio/RtAudio.h>

#include "AudioSession.h"

enum class DeviceRole
{
    Input,
    Output
};

int channelCount(const RtAudio::DeviceInfo &info, DeviceRole role);
bool isUsable(const RtAudio::DeviceInfo &info, DeviceRole role);

struct DeviceOption
{
    unsigned int id = 0;
    RtAudio::DeviceInfo info{};
    bool selected = false;
    bool disabled = false;
};

class DeviceSelector
{
public:
    static std::vector<DeviceOption> makeOptions(const std::vector<DeviceEntry> &entries,
                                                 DeviceRole role,
                                                 std::optional<unsigned int> selectedId);

    static bool combo(const char *label,
                      const std::vector<DeviceOption> &options,
                      DeviceRole role,
                      const std::function<void(unsigned int)> &onSelect);

    static bool list(const char *id,
                     const std::vector<DeviceOption> &options,
                     DeviceRole role,
                     const std::function<void(unsigned int)> &onSelect,
                     ImVec2 size);
};
