// src/main.cpp
#include <iostream>
#include <vector>
#include <string>
#include <limits> 
#include <ios>    
#include <map>
#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>
#include <iomanip> 

#include "AudioManager.h"

// Global flags to signal shutdown from Ctrl+C handler
std::atomic<bool> g_quit_flag(false);

void signalHandler(int signal) {
    (void)signal;
    if (signal == SIGINT) {
        if (g_quit_flag.load()){
            // Detect second Ctrl+C, force exit
            std::cerr << "\nForcing exit!" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::cout << "\nCtrl+C detected, signaling shutdown..." << std::endl;
        g_quit_flag.store(true); // Signal main loop to stop
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
                if (info.name.empty() && info.inputChannels == 0 && info.outputChannels == 0) {
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
        unsigned int bufferFrames;

        if (manager.getCurrentApi() == RtAudio::Api::UNIX_JACK) {
            // For JACK, always request the system default buffer size.
            std::cout << "JACK API in use. Requesting system default buffer size (0)." << std::endl;
            bufferFrames = 0; // Let system decide
        } else {
            // For ALSA, Pulse, or others, use a specific size that works.
            bufferFrames = 1024;
            std::cout << "Non-JACK API in use. Requesting buffer size: " << bufferFrames << std::endl;
        }

        std::cout << "\nAttempting to open monitoring stream..." << std::endl;
        if (manager.openMonitoringStream(inputDeviceId, sampleRate, bufferFrames)) {
            // Stream opened, PitchDetector created
            std::cout << "Stream opened. Attempting to start..." << std::endl;
            if (manager.startStream()) {
                std::cout << "\n--- Pitch Detection Started ---" << std::endl;
                std::cout << "Press Ctrl+C to stop monitoring." << std::endl;
                
                // --- Main Loop ---
                float last_displayed_freq = -1.0f;
                while (!g_quit_flag.load()) {
                    if(!manager.isStreamRunning()){
                        std::cerr << "\nWarning: Stream stopped unexpectedly!" << std::endl;
                        break;
                    }

                    // Get latest pitch from AudioManager
                    float current_freq = manager.getLatestPitchHz();

                    // Display frequency
                    if (current_freq > 0.0f && current_freq != last_displayed_freq) {
                        std::cout << "Detected Pitch (Hz): " << std::fixed << std::setprecision(2)
                                  << current_freq << "        \r";
                        fflush(stdout);
                        last_displayed_freq = current_freq;
                    } else if (current_freq <= 0.0f && last_displayed_freq > 0.0f) {
                         std::cout << "Detected Pitch (Hz): ---.--        \r";
                         fflush(stdout);
                         last_displayed_freq = 0.0f;
                    }

                    // Sleep briefly
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                // --- End of Main Loop ---

                std::cout << "\n--- Stopping Monitoring ---" << std::endl;
                if (!manager.stopStream()) {
                    std::cerr << "Failed to stop stream cleanly." << std::endl;
                }
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