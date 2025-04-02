// src/main.cpp
#include <iostream>
#include <vector>
#include <string>
#include <limits> 
#include <ios>    
#include <map>
#include <csignal>
#include <atomic>

#include "AudioManager.h"

// Global flags to signal shutdown from Ctrl+C handler
std::atomic<bool> g_quit_flag(false);
std::atomic<bool> audio_stream_running_flag(false);

void signalHandler(int signal) {
    (void)signal;
    if (signal == SIGINT && audio_stream_running_flag.load()) {
        std::cout << "\nCtrl+C detected, signaling shutdown..." << std::endl;
        g_quit_flag.store(true);
    }else{
        std::cout << "\nProgram finished." << std::endl;
        exit(0);
    }
}


int main() {
    std::cout << "OpenChordix - RtAudio Input Monitor" << std::endl;
    std::cout << "RtAudio Version: " << RtAudio::getVersion() << std::endl;

    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    // 1. Get available APIs and let user choose
    std::vector<RtAudio::Api> availableApis = AudioManager::getAvailableApis();
    if (availableApis.empty()) {
        std::cerr << "Error: No RtAudio APIs compiled or found!" << std::endl;
        return 1;
    }
    std::cout << "\nPlease choose an audio API:" << std::endl;
    for (size_t i = 0; i < availableApis.size(); ++i) { 
        std::cout << "  " << (i + 1) << ". " << RtAudio::getApiDisplayName(availableApis[i])
                  << " (" << availableApis[i] << ")" << std::endl;
    }
    std::cout << "  " << (availableApis.size() + 1) << ". Auto Select (UNSPECIFIED)" << std::endl;
    int apiChoice = 0;
    RtAudio::Api selectedApi = RtAudio::Api::UNSPECIFIED;
    while (true) {
        std::cout << "Enter choice (1-" << (availableApis.size() + 1) << "): ";
        std::cin >> apiChoice;
        if (std::cin.fail() || apiChoice < 1 || apiChoice > (int)(availableApis.size() + 1)) {
             std::cerr << "Invalid input..." << std::endl;
             std::cin.clear();
             std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else {
            if (apiChoice <= (int)availableApis.size()) {
                selectedApi = availableApis[apiChoice - 1];
            }
            break;
        }
    }
     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


    try {
        // 2. Create the AudioManager instance
        std::cout << "\nInitializing audio manager..." << std::endl;
        AudioManager manager(selectedApi);

        // 3. List devices and let user choose INPUT device
        std::cout << "\nListing available audio devices..." << std::endl;
        if (!manager.listDevices()) {
             std::cerr << "Could not list any devices for the selected API. Exiting." << std::endl;
             return 1;
        }

        unsigned int inputDeviceId = 0;
        bool validInputDevice = false;
        while (!validInputDevice) {
            std::cout << "\nEnter the Device ID of the INPUT device you want to monitor: ";
            std::cin >> inputDeviceId;

            if (std::cin.fail()) {
                std::cerr << "Invalid input. Please enter a number." << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue; // Ask again
            }
             std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Consume newline

            // Validate the chosen ID
            try {
                RtAudio::DeviceInfo info = manager.getDeviceInfo(inputDeviceId);
                if (info.ID == 0) { // Check if getDeviceInfo returned default/empty struct
                     std::cerr << "Device ID " << inputDeviceId << " not found or invalid." << std::endl;
                } else if (info.inputChannels == 0) {
                     std::cerr << "Device ID " << inputDeviceId << " (" << info.name << ") has no input channels." << std::endl;
                } else {
                    std::cout << "Selected input device: " << info.name << " (ID: " << inputDeviceId << ")" << std::endl;
                    validInputDevice = true; // Input is valid
                }
            } catch (const std::runtime_error& e) {
                 // Catch error from AudioManager::getDeviceInfo if audio_ was null
                 std::cerr << "Error validating device ID: " << e.what() << std::endl;
                 // Might indicate a deeper problem, perhaps exit?
                 return 1;
            }
        }


        // 4. Open and Start the Monitoring Stream
        // Use desired sample rate and buffer size (make configurable later)
        unsigned int sampleRate = 48000; // Default rate for interfaces
        unsigned int bufferFrames = 512; // framebuffer

        std::cout << "\nAttempting to open monitoring stream..." << std::endl;
        if (manager.openMonitoringStream(inputDeviceId, sampleRate, bufferFrames)) {
            std::cout << "Stream opened. Attempting to start..." << std::endl;
            if (manager.startStream()) {
                audio_stream_running_flag.store(true);
                std::cout << "\n--- Monitoring Started ---" << std::endl;
                std::cout << "Playing audio from input device ID " << inputDeviceId << std::endl;
                std::cout << "Press Ctrl+C to stop monitoring." << std::endl;

                // Keep running while the stream is active and no quit signal
                while (manager.isStreamRunning() && !g_quit_flag.load()) {
                    // Sleep briefly to avoid busy-waiting
                    #ifdef _WIN32
                        Sleep(100); // Windows sleep (milliseconds)
                    #else
                        usleep(100000); // POSIX sleep (microseconds) -> 100ms
                    #endif
                }

                std::cout << "\n--- Stopping Monitoring ---" << std::endl;
                manager.stopStream();

            } else {
                std::cerr << "Failed to start the audio stream." << std::endl;
            }
            // close the stream !!!
            manager.closeStream();
        } else {
            std::cerr << "Failed to open the audio stream." << std::endl;
        }

    } catch (const std::runtime_error& e) {
         std::cerr << "Fatal Error during AudioManager setup: " << e.what() << std::endl;
         return 1;
    } catch (const std::exception& e) {
         std::cerr << "Fatal Error: An unexpected exception occurred. " << e.what() << std::endl;
         return 1;
    }

    std::cout << "\nProgram finished." << std::endl;
    return 0;
}