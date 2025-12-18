#include "AppController.h"

#include <iostream>

#include "console/ConsoleFlow.h"
#include "GraphicsFlow.h"

AppController::AppController(const std::vector<RtAudio::Api> &apis)
    : audio_({22050, 32000, 44100, 48000, 88200, 96000}, {64, 128, 256, 512, 1024, 2048}), apis_(apis)
{
}

int AppController::run(std::atomic<bool> &quitFlag)
{
    bool windowOk = gfx_.initializeWindowed("OpenChordix");
    bool rendererOk = windowOk && gfx_.initializeRenderer();
    if (!windowOk || !rendererOk)
    {
        std::cerr << "GUI bootstrap failed (" << (windowOk ? "renderer init failed" : "window init failed") << "); falling back to console mode.\n";
        ConsoleFlow console(apis_, noteConverter_);
        return console.run(quitFlag);
    }

    GraphicsFlow graphics(gfx_, audio_, configStore_, noteConverter_, ui_, apis_);
    return graphics.run(quitFlag);
}
