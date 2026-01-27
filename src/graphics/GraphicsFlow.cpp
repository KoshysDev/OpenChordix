#include "GraphicsFlow.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "AudioSetupScene.h"
#include "platform/ImGuiPlatformBridge.h"
#include "IntroScene.h"
#include "MainMenuScene.h"
#include "scenes/SceneCatalog.h"
#include "render/RenderViewIds.h"
#include "SettingsScene.h"
#include "TestScene.h"
#include "TrackSelectScene.h"
#include "TunerScene.h"
#include "devtools/commands/CommandList.h"

namespace openchordix::devtools
{
    void registerSceneCommands(CommandRegistry &registry,
                               std::function<std::vector<std::string>()> listProvider,
                               std::function<bool(std::string_view)> loadHandler);

    void registerModelCommands(CommandRegistry &registry,
                               std::function<bool(std::string_view)> loadHandler,
                               std::function<void()> clearHandler,
                               std::function<std::string()> statusProvider);
}



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
        return std::make_unique<TestScene>(ui_, testSceneModel_);
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
    ImGuiPlatformBridge imguiBridge(gfx_.window());
    imguiBridge.installClipboard();

    SceneId sceneId = SceneId::Intro;
    std::unique_ptr<Scene> currentScene = makeScene(sceneId);
    auto switchTo = [&](SceneId id)
    {
        sceneId = id;
        currentScene = makeScene(id);
    };

    openchordix::devtools::registerSceneCommands(
        devConsole_.registry(),
        []()
        { return SceneCatalog::names(); },
        [&](std::string_view name)
        {
            auto id = SceneCatalog::fromName(name);
            if (!id)
            {
                return false;
            }
            switchTo(*id);
            return true;
        });

    openchordix::devtools::registerModelCommands(
        devConsole_.registry(),
        [&](std::string_view path)
        {
            return testSceneModel_.loadModel(gfx_.modelRenderer(), path);
        },
        [&]()
        {
            testSceneModel_.clearModel();
        },
        [&]()
        {
            return testSceneModel_.status();
        });

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
        imguiBridge.updateKeyboard(input);
        devConsole_.updateToggle(glfwGetKey(gfx_.window(), GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS);

        gfx_.renderer().beginFrame(openchordix::render::kViewIdScene);
        imguiBeginFrame(
            input.mouseX,
            input.mouseY,
            input.buttonMask,
            input.scroll,
            static_cast<uint16_t>(gfx_.config().width),
            static_cast<uint16_t>(gfx_.config().height),
            -1,
            openchordix::render::kViewIdUi);

        currentScene->render(dt, input, gfx_, quitFlag);

        devConsole_.render();

        if (sceneId == SceneId::MainMenu)
        {
            if (auto *menu = dynamic_cast<MainMenuScene *>(currentScene.get()))
            {
                switch (menu->consumeAction())
                {
                case MainMenuScene::Action::OpenTuner:
                    switchTo(SceneId::Tuner);
                    break;
                case MainMenuScene::Action::OpenTrackSelect:
                    switchTo(SceneId::TrackSelect);
                    break;
                case MainMenuScene::Action::OpenSettings:
                    switchTo(SceneId::Settings);
                    break;
                case MainMenuScene::Action::None:
                    break;
                }
            }
        }

        if (currentScene && currentScene->finished())
        {
            switch (sceneId)
            {
            case SceneId::Intro:
                switchTo(afterIntro);
                break;
            case SceneId::AudioSetup:
            {
                AudioConfig config = audio_.currentConfig();
                if (config.isUsable())
                {
                    configStore_.saveAudioConfig(config);
                }
                switchTo(afterAudio);
                break;
            }
            case SceneId::Tuner:
            case SceneId::Settings:
            case SceneId::Test:
            case SceneId::TrackSelect:
                switchTo(SceneId::MainMenu);
                break;
            case SceneId::MainMenu:
                break;
            }
        }

        imguiEndFrame();
        gfx_.renderer().endFrame();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    audio_.stopMonitoring(false);
    testSceneModel_.clearModel();
    imguiDestroy();
    gfx_.shutdown();
    return 0;
}
