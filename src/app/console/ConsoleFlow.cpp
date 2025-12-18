#include "console/ConsoleFlow.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <thread>
#include <cstdio>

#include "ui/DeviceSelector.h"

ConsoleFlow::ConsoleFlow(std::vector<RtAudio::Api> apis, NoteConverter &noteConverter)
    : prompter_(std::move(apis)), noteConverter_(noteConverter)
{
}

int ConsoleFlow::run(std::atomic<bool> &quitFlag)
{
    if (!prompter_.hasApis())
    {
        std::cerr << "Error: No RtAudio APIs compiled or found!" << std::endl;
        return 1;
    }

    RtAudio::Api selectedApi = prompter_.chooseApi();

    try
    {
        AudioManager manager(selectedApi);

        if (!manager.listDevices())
        {
            std::cerr << "Could not list any devices for the selected API. Exiting." << std::endl;
            return 1;
        }

        auto inputDeviceId = prompter_.chooseDevice(manager, DeviceRole::Input, manager.getDefaultInputDeviceId());
        if (!inputDeviceId)
        {
            return 1;
        }

        auto outputDeviceId = prompter_.chooseDevice(manager, DeviceRole::Output, manager.getDefaultOutputDeviceId());
        if (!outputDeviceId)
        {
            return 1;
        }

        unsigned int sampleRate = 48000;
        unsigned int bufferFrames = manager.getCurrentApi() == RtAudio::Api::UNIX_JACK ? 0 : 1024;

        if (manager.openMonitoringStream(*inputDeviceId, *outputDeviceId, sampleRate, bufferFrames))
        {
            if (manager.startStream())
            {
                std::cout << "\n--- Pitch Detection Started ---" << std::endl;
                std::cout << "Press Ctrl+C to stop monitoring." << std::endl;

                float lastDisplayedFreq = -1.0f;

                while (!quitFlag.load())
                {
                    if (!manager.isStreamRunning())
                    {
                        std::cerr << "\nWarning: Stream stopped unexpectedly!" << std::endl;
                        break;
                    }

                    float currentFreq = manager.getLatestPitchHz();
                    if (currentFreq > 10.0f && std::abs(currentFreq - lastDisplayedFreq) > 0.5f)
                    {
                        NoteInfo noteInfo = noteConverter_.getNoteInfo(currentFreq);

                        std::cout << "Freq: " << std::fixed << std::setprecision(1) << std::setw(6) << currentFreq << " Hz "
                                  << "| Note: " << std::left << std::setw(3) << (noteInfo.name + std::to_string(noteInfo.octave))
                                  << "| Cents: " << std::right << std::showpos << std::fixed << std::setprecision(1) << std::setw(6) << noteInfo.cents
                                  << std::noshowpos
                                  << "   \r";
                        fflush(stdout);

                        lastDisplayedFreq = currentFreq;
                    }
                    else if (currentFreq <= 10.0f && lastDisplayedFreq > 0.0f)
                    {
                        std::cout << "Freq:  ---.-- Hz | Note: --- | Cents: ------    \r";
                        fflush(stdout);
                        lastDisplayedFreq = 0.0f;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }

                std::cout << "\n--- Stopping Monitoring ---" << std::endl;
                manager.stopStream();
            }
            manager.closeStream();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal Error during AudioManager setup: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
