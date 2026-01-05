#include "GraphicsFlow.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <thread>

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "AudioSetupScene.h"
#include "IntroScene.h"
#include "MainMenuScene.h"
#include "SettingsScene.h"
#include "TestScene.h"
#include "TrackSelectScene.h"
#include "TunerScene.h"
#include "devtools/commands/CommandList.h"

GraphicsFlow::GraphicsFlow(GraphicsContext &gfx,
                           AudioSession &audio,
                           ConfigStore &configStore,
                           NoteConverter &noteConverter,
                           AnimatedUI &ui,
                           const std::vector<RtAudio::Api> &apis,
                           bool enableDevTools)
    : gfx_(gfx),
      audio_(audio),
      configStore_(configStore),
      noteConverter_(noteConverter),
      ui_(ui),
      apis_(apis),
      devConsole_(enableDevTools)
{
    if (enableDevTools)
    {
        openchordix::devtools::registerDefaultCommands(devConsole_.registry());
    }
}

void GraphicsFlow::configureImGuiStyle()
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

std::unique_ptr<Scene> GraphicsFlow::makeScene(SceneId id)
{
    switch (id)
    {
    case SceneId::Intro:
        return std::make_unique<IntroScene>();
    case SceneId::AudioSetup:
        return std::make_unique<AudioSetupScene>(audio_, noteConverter_, ui_);
    case SceneId::MainMenu:
        return std::make_unique<MainMenuScene>(ui_);
    case SceneId::TrackSelect:
        return std::make_unique<TrackSelectScene>(ui_);
    case SceneId::Tuner:
        return std::make_unique<TunerScene>(audio_, ui_);
    case SceneId::Settings:
        return std::make_unique<SettingsScene>(audio_, ui_);
    case SceneId::Test:
        return std::make_unique<TestScene>(ui_);
    }
    return nullptr;
}

int GraphicsFlow::run(std::atomic<bool> &quitFlag)
{
    devConsole_.setQuitCallback([&quitFlag]()
                                { quitFlag.store(true); });

    std::optional<AudioConfig> savedConfig = configStore_.loadAudioConfig();
    RtAudio::Api initialApi = apis_.front();
    bool configApiSupported = false;
    if (savedConfig)
    {
        auto apiIt = std::find(apis_.begin(), apis_.end(), savedConfig->api);
        if (apiIt != apis_.end())
        {
            initialApi = savedConfig->api;
            configApiSupported = true;
        }
    }
    if (savedConfig && !configApiSupported)
    {
        savedConfig.reset();
    }

    audio_.setApi(initialApi);
    audio_.refreshDevices(initialApi);

    bool useSetupScene = true;
    if (savedConfig)
    {
        bool configApplied = audio_.applyConfig(*savedConfig);
        if (configApplied && savedConfig->isUsable())
        {
            if (audio_.startMonitoring())
            {
                useSetupScene = false;
            }
            else
            {
                audio_.setStatus("Failed to start with saved audio config. Please reconfigure.");
                audio_.stopMonitoring(false);
            }
        }
        else
        {
            audio_.setStatus("Saved audio config does not match available devices.");
        }
    }

    imguiCreate(20.0f);
    configureImGuiStyle();

    SceneId sceneId = SceneId::Intro;
    std::unique_ptr<Scene> currentScene = makeScene(sceneId);
    auto switchTo = [&](SceneId id)
    {
        sceneId = id;
        currentScene = makeScene(id);
    };

    SceneId afterIntro = useSetupScene ? SceneId::AudioSetup : SceneId::MainMenu;
    SceneId afterAudio = SceneId::MainMenu;

    auto lastClock = std::chrono::steady_clock::now();

    while (!quitFlag.load() && !gfx_.shouldClose())
    {
        if (!currentScene)
        {
            break;
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastClock).count();
        lastClock = now;
        dt = std::clamp(dt, 0.0f, 0.1f);

        audio_.updatePitch(noteConverter_);
        ui_.beginFrame(dt);
        FrameInput input = gfx_.pollFrame();
        devConsole_.updateToggle(glfwGetKey(gfx_.window(), GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS);

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

        currentScene->render(dt, input, gfx_, quitFlag);

        devConsole_.render();

        if (sceneId == SceneId::MainMenu)
        {
            if (auto *menu = dynamic_cast<MainMenuScene *>(currentScene.get()))
            {
                MainMenuScene::Action action = menu->consumeAction();
                if (action == MainMenuScene::Action::OpenTuner)
                {
                    switchTo(SceneId::Tuner);
                }
                else if (action == MainMenuScene::Action::OpenTrackSelect)
                {
                    switchTo(SceneId::TrackSelect);
                }
                else if (action == MainMenuScene::Action::OpenSettings)
                {
                    switchTo(SceneId::Settings);
                }
            }
        }

        if (currentScene && currentScene->finished())
        {
            if (sceneId == SceneId::Intro)
            {
                switchTo(afterIntro);
            }
            else if (sceneId == SceneId::AudioSetup)
            {
                AudioConfig config = audio_.currentConfig();
                if (config.isUsable())
                {
                    configStore_.saveAudioConfig(config);
                }
                switchTo(afterAudio);
            }
            else if (sceneId == SceneId::Tuner)
            {
                switchTo(SceneId::MainMenu);
            }
            else if (sceneId == SceneId::Settings)
            {
                switchTo(SceneId::MainMenu);
            }
            else if (sceneId == SceneId::Test)
            {
                switchTo(SceneId::MainMenu);
            }
            else if (sceneId == SceneId::TrackSelect)
            {
                switchTo(SceneId::MainMenu);
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
