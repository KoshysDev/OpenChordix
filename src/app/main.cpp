#include <iostream>
#include <csignal>
#include <atomic>
#include <vector>
#include <string_view>

#include <rtaudio/RtAudio.h>

#include "AppController.h"

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

int main(int argc, char **argv)
{
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
        std::cerr << "Error: No RtAudio APIs compiled or found!" << std::endl;
        return 1;
    }

    AppController app(apis, enableDevTools);
    return app.run(g_quit_flag);
}
