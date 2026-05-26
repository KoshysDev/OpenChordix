#pragma once

#include <rtaudio/RtAudio.h>

struct DeviceEntry
{
    unsigned int id = 0;
    RtAudio::DeviceInfo info{};
};
