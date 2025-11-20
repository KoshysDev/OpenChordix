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
#include <cstdint>
#include <cstdlib>
#include <algorithm>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "AudioManager.h"
#include "NoteConverter.h"
#include "Renderer.h"

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
    std::cout << "OpenChordix" << std::endl;
    std::cout << "RtAudio Version: " << RtAudio::getVersion() << std::endl;

    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    GLFWwindow* window = nullptr;

    // Prefer Wayland when available to avoid Xwayland fallbacks.
    if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND)) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    }

    if (!glfwInit()) {
        std::cerr << "Warning: Failed to initialize GLFW. Running headless." << std::endl;
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(1280, 720, "OpenChordix", nullptr, nullptr);
        if (!window) {
            std::cerr << "Warning: Failed to create GLFW window. Running headless." << std::endl;
            glfwTerminate();
        }
    }

    openchordix::Renderer renderer;
    openchordix::RendererConfig rendererConfig;
    bool startedWithWindow = window != nullptr;

    if (window) {
        int fbWidth = 0;
        int fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        rendererConfig.width = static_cast<uint32_t>(fbWidth);
        rendererConfig.height = static_cast<uint32_t>(fbHeight);

        void* wlDisplay = glfwGetWaylandDisplay();
        void* wlSurface = glfwGetWaylandWindow(window);
        const bool hasWaylandHandles = wlDisplay != nullptr && wlSurface != nullptr;

        if (hasWaylandHandles) {
            rendererConfig.headless = false;
            rendererConfig.type = bgfx::RendererType::Vulkan; // prefer Vulkan on Wayland
            rendererConfig.nativeDisplayType = wlDisplay;
            rendererConfig.nativeWindowHandle = wlSurface;
            rendererConfig.nativeHandleType = bgfx::NativeWindowHandleType::Wayland;
        } else {
            rendererConfig.headless = true;
            std::cerr << "Wayland handles unavailable; destroying window and running headless." << std::endl;
            glfwDestroyWindow(window);
            window = nullptr;
            startedWithWindow = false;
        }
    }

    if (!renderer.initialize(rendererConfig)) {
        std::cerr << "Warning: Failed to initialize BGFX renderer. Continuing without graphics.\n";
    }

    // Audio API selection
    std::vector<RtAudio::Api> availableApis = AudioManager::getAvailableApis();
    if (availableApis.empty()) {
        std::cerr << "Error: No RtAudio APIs compiled or found!" << std::endl;
        return 1;
    }

    const bool graphicsMode = startedWithWindow && renderer.isInitialized() && !renderer.config().headless;

    RtAudio::Api selectedApi = RtAudio::Api::UNSPECIFIED;
    if (graphicsMode) {
        // Prefer ALSA > Pulse > JACK > fallback
        auto pick = [&](RtAudio::Api api) {
            return std::find(availableApis.begin(), availableApis.end(), api) != availableApis.end();
        };
        if (pick(RtAudio::Api::LINUX_ALSA)) {
            selectedApi = RtAudio::Api::LINUX_ALSA;
        } else if (pick(RtAudio::Api::LINUX_PULSE)) {
            selectedApi = RtAudio::Api::LINUX_PULSE;
        } else if (pick(RtAudio::Api::UNIX_JACK)) {
            selectedApi = RtAudio::Api::UNIX_JACK;
        } else {
            selectedApi = availableApis.front();
        }
        std::cout << "Auto-selecting audio API: " << RtAudio::getApiDisplayName(selectedApi) << std::endl;
    } else {
        std::cout << "\nPlease choose an audio API:" << std::endl;
        for (size_t i = 0; i < availableApis.size(); ++i) { 
            std::cout << "  " << (i + 1) << ". " << RtAudio::getApiDisplayName(availableApis[i])
                      << " (" << availableApis[i] << ")" << std::endl;
        }
        std::cout << "  " << (availableApis.size() + 1) << ". Auto Select (UNSPECIFIED)" << std::endl;
        int apiChoice = 0;
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
    }


    try {
        // 2. Create the AudioManager instance
        std::cout << "\nInitializing audio manager..." << std::endl;
        AudioManager manager(selectedApi);

        // 3. List devices and choose INPUT device
        std::cout << "\nListing available audio devices..." << std::endl;
        if (!manager.listDevices()) {
             std::cerr << "Could not list any devices for the selected API. Exiting." << std::endl;
             return 1;
        }

        unsigned int inputDeviceId = manager.getDefaultInputDeviceId();
        bool validInputDevice = false;

        if (graphicsMode) {
            try {
                RtAudio::DeviceInfo info = manager.getDeviceInfo(inputDeviceId);
                if (info.inputChannels > 0) {
                    std::cout << "Auto-selected input device: " << info.name << " (ID: " << inputDeviceId << ")" << std::endl;
                    validInputDevice = true;
                } else {
                    std::cerr << "Default input device has no input channels. Skipping audio start." << std::endl;
                }
            } catch (const std::runtime_error& e) {
                std::cerr << "Error validating default device: " << e.what() << std::endl;
            }
        } else {
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
        }


        if (!validInputDevice) {
            std::cerr << "No valid input device available; audio will be disabled for this run." << std::endl;

            if (renderer.isInitialized() && window) {
                int lastFbWidth = static_cast<int>(rendererConfig.width);
                int lastFbHeight = static_cast<int>(rendererConfig.height);

                while (!g_quit_flag.load() && !glfwWindowShouldClose(window)) {
                    glfwPollEvents();
                    int fbw = 0;
                    int fbh = 0;
                    glfwGetFramebufferSize(window, &fbw, &fbh);
                    if ((fbw > 0 && fbh > 0) && (fbw != lastFbWidth || fbh != lastFbHeight)) {
                        renderer.resize(static_cast<uint32_t>(fbw), static_cast<uint32_t>(fbh));
                        lastFbWidth = fbw;
                        lastFbHeight = fbh;
                    }

                    if (!renderer.config().headless) {
                        bgfx::dbgTextClear();
                        bgfx::dbgTextPrintf(1, 1, 0x0f, "OpenChordix - Wayland mode");
                    }
                    renderer.frame();

                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                }
            }
        } else {
            // 4. Open and Start the Monitoring Stream
            // Use desired sample rate and buffer size (make configurable later)
            unsigned int sampleRate = 48000; // Default rate for interfaces
            unsigned int bufferFrames;
            NoteConverter noteConverter;

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
                    if (!graphicsMode) {
                        std::cout << "Press Ctrl+C to stop monitoring." << std::endl;
                    }
                    
                    // --- Main Loop ---
                    float last_displayed_freq = -1.0f;
                    std::string last_note_name = "---";

                    int lastFbWidth = static_cast<int>(rendererConfig.width);
                    int lastFbHeight = static_cast<int>(rendererConfig.height);

                    while (!g_quit_flag.load() && (!window || !glfwWindowShouldClose(window))) {
                        if(!manager.isStreamRunning()){
                            std::cerr << "\nWarning: Stream stopped unexpectedly!" << std::endl;
                            break;
                        }

                        if (window) {
                            glfwPollEvents();
                            int fbw = 0;
                            int fbh = 0;
                            glfwGetFramebufferSize(window, &fbw, &fbh);
                            if ((fbw > 0 && fbh > 0) && (fbw != lastFbWidth || fbh != lastFbHeight)) {
                                renderer.resize(static_cast<uint32_t>(fbw), static_cast<uint32_t>(fbh));
                                lastFbWidth = fbw;
                                lastFbHeight = fbh;
                            }
                        }

                        // Get latest pitch from AudioManager
                        float current_freq = manager.getLatestPitchHz();

                        // Only update display if frequency seems valid and has changed
                        if (current_freq > 10.0f && std::abs(current_freq - last_displayed_freq) > 0.5f) {
                            // Convert frequency to note info
                            NoteInfo noteInfo = noteConverter.getNoteInfo(current_freq);

                            // Display formatted info
                            std::cout << "Freq: " << std::fixed << std::setprecision(1) << std::setw(6) << current_freq << " Hz "
                                      << "| Note: " << std::left << std::setw(3) << (noteInfo.name + std::to_string(noteInfo.octave))
                                      << "| Cents: " << std::right << std::showpos << std::fixed << std::setprecision(1) << std::setw(6) << noteInfo.cents
                                      << std::noshowpos 
                                      << "   \r";
                            fflush(stdout);

                            last_displayed_freq = current_freq;
                            last_note_name = noteInfo.name + std::to_string(noteInfo.octave);

                        } else if (current_freq <= 10.0f && last_displayed_freq > 0.0f) {
                             // If pitch becomes invalid, display placeholders
                             std::cout << "Freq:  ---.-- Hz | Note: --- | Cents: ------    \r";
                             fflush(stdout);
                             last_displayed_freq = 0.0f; // Reset tracking
                             last_note_name = "---";
                        }

                        // Sleep briefly
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));

                        if (renderer.isInitialized()) {
                            if (!renderer.config().headless) {
                                bgfx::dbgTextClear();
                                bgfx::dbgTextPrintf(1, 1, 0x0f, "OpenChordix - Wayland mode");
                            }
                            renderer.frame();
                        }
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
        }

    } catch (const std::runtime_error& e) {
         std::cerr << "Fatal Error during AudioManager setup: " << e.what() << std::endl;
         return 1;
    } catch (const std::exception& e) {
         std::cerr << "Fatal Error: An unexpected exception occurred. " << e.what() << std::endl;
         return 1;
    }

    renderer.shutdown();

    if (window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    std::cout << "\nProgram finished." << std::endl;
    return 0;
}
