// src/app/main.cpp
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
#include <memory>

#include "AudioManager.h"
#include "NoteConverter.h"
#include "gui.h"

std::unique_ptr<GUI> g_gui_ptr = nullptr; // Use unique_ptr for ownership

void signalHandler(int signal) {
    (void)signal;
    if (signal == SIGINT) {
        std::cout << "\nCtrl+C detected, requesting shutdown..." << std::endl;
        if (g_gui_ptr) {
             g_gui_ptr->requestShutdown(); // Signal the GUI loop to stop
        }
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << "OpenChordix - Initializing..." << std::endl;
    std::cout << "RtAudio Version: " << RtAudio::getVersion() << std::endl;

    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    // --- GUI Initialization ---
    g_gui_ptr = std::make_unique<GUI>(); // Create the GUI object
    const int windowWidth = 1280;
    const int windowHeight = 720;

    if (!g_gui_ptr->init(windowWidth, windowHeight, "OpenChordix")) {
        std::cerr << "Failed to initialize GUI. Exiting." << std::endl;
        g_gui_ptr.reset(); // Clean up partial init
        return 1;
    }

// --- Audio Initialization ---
std::cout << "\n--- Initializing Audio ---" << std::endl;
std::unique_ptr<AudioManager> audioManager = nullptr; // Keep scope local to main
std::unique_ptr<NoteConverter> noteConverter = nullptr;
bool audioInitialized = false;

try {
    // 1. Choose API 
    std::vector<RtAudio::Api> availableApis = AudioManager::getAvailableApis();
    RtAudio::Api selectedApi = RtAudio::Api::UNSPECIFIED; // Default to auto
    if (!availableApis.empty()) {
        std::cout << "Available Audio APIs:" << std::endl;
         for (size_t i = 0; i < availableApis.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << RtAudio::getApiDisplayName(availableApis[i]) << std::endl;
        }
         std::cout << "  " << (availableApis.size() + 1) << ". Auto Select (UNSPECIFIED)" << std::endl;
         std::cout << "Using Auto-Select API." << std::endl;
    } else {
        std::cerr << "Warning: No specific RtAudio APIs reported." << std::endl;
    }

    // 2. Create AudioManager
    audioManager = std::make_unique<AudioManager>(selectedApi);

    // 3. List Devices & Choose Input
    std::cout << "\nListing audio devices for API: " << RtAudio::getApiDisplayName(audioManager->getCurrentApi()) << "..." << std::endl;
    if (!audioManager->listDevices()) {
         std::cerr << "Warning: Could not list audio devices. Attempting default." << std::endl;
         // Continue and try default, or exit if listing is critical
    }

    unsigned int inputDeviceId = audioManager->getDefaultInputDeviceId();
    std::cout << "Attempting to use default input device ID: " << inputDeviceId << std::endl;

    // Check if a valid default device was found
    if (inputDeviceId == 0 && audioManager->getCurrentApi() != RtAudio::Api::RTAUDIO_DUMMY) {
         RtAudio::DeviceInfo info = audioManager->getDeviceInfo(inputDeviceId); // Attempt to get info for ID 0
         if (info.inputChannels == 0) { // Check if device ID 0 is actually valid input
             throw std::runtime_error("No default input device found or default device has no input channels.");
         }
         // It seems ID 0 might be valid on some systems, proceed cautiously.
         std::cout << "Warning: Default input device ID is 0. Proceeding..." << std::endl;
    } else {
         RtAudio::DeviceInfo info = audioManager->getDeviceInfo(inputDeviceId);
         if (info.inputChannels == 0) {
              throw std::runtime_error("Selected input device (ID: " + std::to_string(inputDeviceId) + ") has no input channels.");
         }
         std::cout << "Using Input Device: " << info.name << " (ID: " << inputDeviceId << ")" << std::endl;
    }


    // 4. Create Note Converter
    noteConverter = std::make_unique<NoteConverter>(); // Default A4=440Hz

    // 5. Open and Start Stream
    unsigned int sampleRate = 48000; // Or choose based on device info
    unsigned int bufferFrames = 1024; // Or 0 for JACK, or choose based on device
     if (audioManager->getCurrentApi() == RtAudio::Api::UNIX_JACK) {
        bufferFrames = 0; // Let JACK decide buffer size
         std::cout << "JACK API detected, requesting default buffer size (0)." << std::endl;
    } else {
         std::cout << "Requesting buffer size: " << bufferFrames << std::endl;
    }

    std::cout << "\nAttempting to open audio stream..." << std::endl;
    if (audioManager->openMonitoringStream(inputDeviceId, sampleRate, bufferFrames)) {
        std::cout << "Audio stream opened. Attempting to start..." << std::endl;
        if (audioManager->startStream()) {
            std::cout << "Audio stream started successfully." << std::endl;
            audioInitialized = true; // Mark audio as ready
        } else {
            std::cerr << "Failed to start audio stream." << std::endl;
            audioManager->closeStream(); // Clean up opened stream
        }
    } else {
        std::cerr << "Failed to open audio stream." << std::endl;
    }

} catch (const std::exception& e) {
    // Catch RtAudio errors or other standard exceptions
    std::cerr << "Error during Audio Initialization: " << e.what() << std::endl;
    std::cerr << "Proceeding without audio." << std::endl;
    audioManager.reset(); // Ensure cleanup if partially initialized
    noteConverter.reset();
    audioInitialized = false;
}

    // --- Run Main Loop ---
    std::cout << "\n--- Starting GUI Main Loop ---" << std::endl;
    g_gui_ptr->run();


    // --- Shutdown ---
    std::cout << "Application exiting..." << std::endl;

    // Audio Shutdown
    if (audioManager) { // Check if manager was created
        std::cout << "Shutting down audio..." << std::endl;
        if (audioManager->isStreamOpen()) {
            if (audioManager->isStreamRunning()) {
                std::cout << "Stopping audio stream..." << std::endl;
                audioManager->stopStream();
            }
            std::cout << "Closing audio stream..." << std::endl;
            audioManager->closeStream();
        }
        audioManager.reset(); // Explicitly release resources
        noteConverter.reset(); // Release note converter
        std::cout << "Audio shutdown complete." << std::endl;
    } else {
        std::cout << "Audio was not initialized, skipping audio shutdown." << std::endl;
    }

    std::cout << "Cleanup complete. Goodbye!" << std::endl;

    return 0;
}