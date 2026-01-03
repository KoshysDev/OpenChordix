#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <rtaudio/RtAudio.h>

#include "audio/AudioManager.h"
#include "ui/DeviceSelector.h"

class ConsolePrompter
{
public:
    explicit ConsolePrompter(std::vector<RtAudio::Api> apis);

    RtAudio::Api chooseApi() const;
    std::optional<unsigned int> chooseDevice(AudioManager &manager, DeviceRole role, unsigned int defaultId) const;
    bool hasApis() const { return !apis_.empty(); }

private:
    std::vector<RtAudio::Api> apis_;
};
