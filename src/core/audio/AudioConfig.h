#pragma once

#include <rtaudio/RtAudio.h>

struct AudioConfig
{
    RtAudio::Api api = RtAudio::Api::UNSPECIFIED;
    unsigned int inputDeviceId = 0;
    unsigned int outputDeviceId = 0;
    unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 1024;

    bool isUsable() const
    {
        return inputDeviceId != 0 && outputDeviceId != 0 && sampleRate > 0 && bufferFrames > 0;
    }
};
