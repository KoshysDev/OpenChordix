#pragma once

#include <string>

class RtAudio
{
public:
    enum class Api
    {
        UNSPECIFIED = 0,
        WINDOWS_DS = 1,
        UNIX_JACK = 2,
    };

    struct DeviceInfo
    {
        std::string name;
        unsigned int inputChannels = 0;
        unsigned int outputChannels = 0;
    };
};
