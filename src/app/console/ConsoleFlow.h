#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include <rtaudio/RtAudio.h>

#include "AudioManager.h"
#include "NoteConverter.h"
#include "console/ConsolePrompter.h"

class ConsoleFlow
{
public:
    ConsoleFlow(std::vector<RtAudio::Api> apis, NoteConverter &noteConverter);
    int run(std::atomic<bool> &quitFlag);

private:
    ConsolePrompter prompter_;
    NoteConverter &noteConverter_;
};
