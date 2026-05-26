#pragma once

#include <ostream>
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

    static std::string getApiDisplayName(Api api)
    {
        switch (api)
        {
        case Api::WINDOWS_DS:
            return "Windows DirectSound";
        case Api::UNIX_JACK:
            return "JACK";
        case Api::UNSPECIFIED:
        default:
            return "Auto Select";
        }
    }

    static std::string getVersion()
    {
        return "stub-1.0";
    }
};

inline std::ostream &operator<<(std::ostream &stream, RtAudio::Api api)
{
    return stream << static_cast<int>(api);
}
