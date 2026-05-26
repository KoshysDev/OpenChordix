#pragma once

#include <rtaudio/RtAudio.h>

enum class DeviceRole
{
    Input,
    Output
};

inline int channelCount(const RtAudio::DeviceInfo &info, DeviceRole role)
{
    return role == DeviceRole::Input
               ? static_cast<int>(info.inputChannels)
               : static_cast<int>(info.outputChannels);
}
