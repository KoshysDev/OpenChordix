#include <iostream>
#include <csignal>
#include <atomic>
#include <vector>
#include <string_view>

#include <rtaudio/RtAudio.h>

#include "AnimatedUI.h"
#include "ConfigStore.h"
#include "GraphicsContext.h"
#include "GraphicsFlow.h"
#include "NoteConverter.h"
#include "audio/AudioSession.h"
#include <console/ConsoleFlow.h>

#ifndef OPENCHORDIX_VERSION
#define OPENCHORDIX_VERSION "0.0.0"
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
extern int __argc;
extern char **__argv;
#endif

// Global flags to signal shutdown from Ctrl+C handler
std::atomic<bool> g_quit_flag(false);

void signalHandler(int signal)
{
    (void)signal;
    if (signal == SIGINT)
    {
        if (g_quit_flag.load())
        {
            std::cerr << "\nForcing exit!" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::cout << "\nCtrl+C detected, signaling shutdown..." << std::endl;
        g_quit_flag.store(true);
    }
}

namespace
{
    int runApplication(const std::vector<RtAudio::Api> &apis,
                       bool enableDevTools,
                       std::atomic<bool> &quitFlag)
    {
        GraphicsContext graphicsContext;
        AudioSession audioSession({22050, 32000, 44100, 48000, 88200, 96000},
                                  {64, 128, 256, 512, 1024, 2048});
        ConfigStore configStore;
        NoteConverter noteConverter;
        AnimatedUI ui;

        const bool windowOk = graphicsContext.initializeWindowed("OpenChordix");
        const bool rendererOk = windowOk && graphicsContext.initializeRenderer();
        if (!windowOk || !rendererOk)
        {
            std::cerr << "GUI bootstrap failed ("
                      << (windowOk ? "renderer init failed" : "window init failed")
                      << "); falling back to console mode.\n";
            ConsoleFlow console(apis, noteConverter);
            return console.run(quitFlag);
        }

        GraphicsFlow graphics(
            graphicsContext,
            audioSession,
            configStore,
            noteConverter,
            ui,
            apis,
            enableDevTools);
        return graphics.run(quitFlag);
    }
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string_view(argv[i]) == "--version")
        {
            std::cout << "OpenChordix " << OPENCHORDIX_VERSION << std::endl;
            return 0;
        }
    }

    std::cout << "OpenChordix" << std::endl;
    std::cout << "RtAudio Version: " << RtAudio::getVersion() << std::endl;

    signal(SIGINT, signalHandler);

    bool enableDevTools = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);
        if (arg == "-debug" || arg == "--debug")
        {
            enableDevTools = true;
        }
    }

    std::vector<RtAudio::Api> apis = AudioManager::getAvailableApis();
    if (apis.empty())
    {
        std::cerr << "Error: No usable RtAudio APIs found. Check audio backend dependencies." << std::endl;
        return 1;
    }

    return runApplication(apis, enableDevTools, g_quit_flag);
}

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main(__argc, __argv);
}
#endif
