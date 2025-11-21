#include "AppController.h"

#include <iostream>
#include <iomanip>
#include <thread>
#include <limits>
#include <sstream>
#include <algorithm>

#include <imgui/imgui.h>

#include "IntroScene.h"
#include "AudioSetupScene.h"
#include "MainMenuScene.h"
#include "Scene.h"

AppController::AppController(const std::vector<RtAudio::Api> &apis)
    : audio_({22050, 32000, 44100, 48000, 88200, 96000}, {64, 128, 256, 512, 1024, 2048}), apis_(apis)
{
}

int AppController::run(std::atomic<bool> &quitFlag)
{
    if (!gfx_.initializeWindowed("OpenChordix") || !gfx_.initializeRenderer())
    {
        return runConsoleFlow(quitFlag);
    }

    return runGraphicsFlow(quitFlag);
}

int AppController::runConsoleFlow(std::atomic<bool> &quitFlag)
{
    if (apis_.empty())
    {
        std::cerr << "Error: No RtAudio APIs compiled or found!" << std::endl;
        return 1;
    }

    RtAudio::Api selectedApi = RtAudio::Api::UNSPECIFIED;
    std::cout << "\nPlease choose an audio API:" << std::endl;
    for (size_t i = 0; i < apis_.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << RtAudio::getApiDisplayName(apis_[i]) << " (" << apis_[i] << ")" << std::endl;
    }
    std::cout << "  " << (apis_.size() + 1) << ". Auto Select (UNSPECIFIED)" << std::endl;
    int apiChoice = 0;
    while (true)
    {
        std::cout << "Enter choice (1-" << (apis_.size() + 1) << "): ";
        std::cin >> apiChoice;
        if (std::cin.fail() || apiChoice < 1 || apiChoice > (int)(apis_.size() + 1))
        {
            std::cerr << "Invalid input..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else
        {
            if (apiChoice <= (int)apis_.size())
            {
                selectedApi = apis_[apiChoice - 1];
            }
            break;
        }
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    try
    {
        AudioManager manager(selectedApi);

        if (!manager.listDevices())
        {
            std::cerr << "Could not list any devices for the selected API. Exiting." << std::endl;
            return 1;
        }

        unsigned int inputDeviceId = manager.getDefaultInputDeviceId();
        bool validInputDevice = false;

        while (!validInputDevice)
        {
            std::cout << "\nEnter the Device ID of the INPUT device you want to monitor: ";
            std::cin >> inputDeviceId;

            if (std::cin.fail())
            {
                std::cerr << "Invalid input. Please enter a number." << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            try
            {
                RtAudio::DeviceInfo info = manager.getDeviceInfo(inputDeviceId);
                if (info.name.empty() && info.inputChannels == 0 && info.outputChannels == 0)
                {
                    std::cerr << "Device ID " << inputDeviceId << " not found or invalid." << std::endl;
                }
                else if (info.inputChannels == 0)
                {
                    std::cerr << "Device ID " << inputDeviceId << " (" << info.name << ") has no input channels." << std::endl;
                }
                else
                {
                    std::cout << "Selected input device: " << info.name << " (ID: " << inputDeviceId << ")" << std::endl;
                    validInputDevice = true;
                }
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << "Error validating device ID: " << e.what() << std::endl;
                return 1;
            }
        }

        if (!validInputDevice)
        {
            std::cerr << "No valid input device available; audio will be disabled for this run." << std::endl;
            return 1;
        }

        unsigned int sampleRate = 48000;
        unsigned int bufferFrames = manager.getCurrentApi() == RtAudio::Api::UNIX_JACK ? 0 : 1024;

        if (manager.openMonitoringStream(inputDeviceId, sampleRate, bufferFrames))
        {
            if (manager.startStream())
            {
                std::cout << "\n--- Pitch Detection Started ---" << std::endl;
                std::cout << "Press Ctrl+C to stop monitoring." << std::endl;

                float last_displayed_freq = -1.0f;

                while (!quitFlag.load())
                {
                    if (!manager.isStreamRunning())
                    {
                        std::cerr << "\nWarning: Stream stopped unexpectedly!" << std::endl;
                        break;
                    }

                    float current_freq = manager.getLatestPitchHz();
                    if (current_freq > 10.0f && std::abs(current_freq - last_displayed_freq) > 0.5f)
                    {
                        NoteInfo noteInfo = noteConverter_.getNoteInfo(current_freq);

                        std::cout << "Freq: " << std::fixed << std::setprecision(1) << std::setw(6) << current_freq << " Hz "
                                  << "| Note: " << std::left << std::setw(3) << (noteInfo.name + std::to_string(noteInfo.octave))
                                  << "| Cents: " << std::right << std::showpos << std::fixed << std::setprecision(1) << std::setw(6) << noteInfo.cents
                                  << std::noshowpos
                                  << "   \r";
                        fflush(stdout);

                        last_displayed_freq = current_freq;
                    }
                    else if (current_freq <= 10.0f && last_displayed_freq > 0.0f)
                    {
                        std::cout << "Freq:  ---.-- Hz | Note: --- | Cents: ------    \r";
                        fflush(stdout);
                        last_displayed_freq = 0.0f;
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

void AppController::configureImGuiStyle()
{
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 1.0f;
    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.10f, 0.95f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.10f, 0.13f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.15f, 0.20f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.17f, 0.23f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.32f, 0.44f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.20f, 0.30f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.14f, 0.18f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.22f, 0.29f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.26f, 0.35f, 1.0f);
}

int AppController::runGraphicsFlow(std::atomic<bool> &quitFlag)
{
    audio_.refreshDevices(apis_.front());

    imguiCreate(20.0f);
    configureImGuiStyle();

    std::vector<std::unique_ptr<Scene>> scenes;
    scenes.emplace_back(std::make_unique<IntroScene>(1.2f));
    scenes.emplace_back(std::make_unique<AudioSetupScene>(audio_, noteConverter_, ui_));
    scenes.emplace_back(std::make_unique<MainMenuScene>());

    size_t currentIndex = 0;
    auto lastClock = std::chrono::steady_clock::now();

    while (!quitFlag.load() && !gfx_.shouldClose())
    {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastClock).count();
        lastClock = now;
        dt = std::clamp(dt, 0.0f, 0.1f);

        FrameInput input = gfx_.pollFrame();

        gfx_.renderer().beginFrame(gfx_.config().viewId);
        imguiBeginFrame(
            input.mouseX,
            input.mouseY,
            input.buttonMask,
            input.scroll,
            static_cast<uint16_t>(gfx_.config().width),
            static_cast<uint16_t>(gfx_.config().height),
            -1,
            gfx_.config().viewId);

        if (currentIndex < scenes.size())
        {
            scenes[currentIndex]->render(dt, input, gfx_, quitFlag);
            if (scenes[currentIndex]->finished() && currentIndex + 1 < scenes.size())
            {
                currentIndex++;
            }
        }

        imguiEndFrame();
        gfx_.renderer().endFrame();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    audio_.stopMonitoring(false);
    imguiDestroy();
    gfx_.shutdown();
    return 0;
}
