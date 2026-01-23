#include "GraphicsFlow.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <bgfx/bgfx.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "AudioSetupScene.h"
#include "IntroScene.h"
#include "MainMenuScene.h"
#include "SettingsScene.h"
#include "TestScene.h"
#include "TrackSelectScene.h"
#include "TunerScene.h"
#include "assets/GltfAsset.h"
#include "assets/GltfLoader.cpp"
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

namespace
{
    std::string toLower(std::string_view text)
    {
        std::string out(text);
        for (char &ch : out)
        {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return out;
    }

    std::optional<GraphicsFlow::SceneId> sceneIdFromName(std::string_view name)
    {
        const std::string id = toLower(name);
        if (id == "intro")
        {
            return GraphicsFlow::SceneId::Intro;
        }
        if (id == "audio" || id == "audio-setup")
        {
            return GraphicsFlow::SceneId::AudioSetup;
        }
        if (id == "menu" || id == "main" || id == "main-menu")
        {
            return GraphicsFlow::SceneId::MainMenu;
        }
        if (id == "track" || id == "track-select")
        {
            return GraphicsFlow::SceneId::TrackSelect;
        }
        if (id == "tuner")
        {
            return GraphicsFlow::SceneId::Tuner;
        }
        if (id == "settings")
        {
            return GraphicsFlow::SceneId::Settings;
        }
        if (id == "test")
        {
            return GraphicsFlow::SceneId::Test;
        }
        return std::nullopt;
    }

    std::vector<std::string> sceneNames()
    {
        return {"intro", "audio", "menu", "track", "tuner", "settings", "test"};
    }

    void updateImGuiKeyboard(GLFWwindow *window, const FrameInput &input)
    {
        ImGuiIO &io = ImGui::GetIO();

        io.AddKeyEvent(ImGuiMod_Shift, glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiMod_Ctrl, glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiMod_Alt, glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiMod_Super, glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS);

        io.AddKeyEvent(ImGuiKey_Tab, glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_LeftArrow, glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_RightArrow, glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_UpArrow, glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_DownArrow, glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_PageUp, glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_PageDown, glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Home, glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_End, glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Insert, glfwGetKey(window, GLFW_KEY_INSERT) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Delete, glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Backspace, glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Space, glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Enter, glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_KeypadEnter, glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Escape, glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_A, glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_C, glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_V, glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_X, glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Y, glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS);
        io.AddKeyEvent(ImGuiKey_Z, glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS);

        for (uint32_t codepoint : input.inputChars)
        {
            io.AddInputCharacter(codepoint);
        }
    }

    const char *getClipboardText(ImGuiContext * /*ctx*/)
    {
        auto *window = static_cast<GLFWwindow *>(ImGui::GetPlatformIO().Platform_ClipboardUserData);
        return window ? glfwGetClipboardString(window) : nullptr;
    }

    void setClipboardText(ImGuiContext * /*ctx*/, const char *text)
    {
        auto *window = static_cast<GLFWwindow *>(ImGui::GetPlatformIO().Platform_ClipboardUserData);
        if (window)
        {
            glfwSetClipboardString(window, text ? text : "");
        }
    }

    void destroyModelTextures(std::vector<bgfx::TextureHandle> &textures)
    {
        for (auto handle : textures)
        {
            if (bgfx::isValid(handle))
            {
                bgfx::destroy(handle);
            }
        }
        textures.clear();
    }

    void drawModelOverlay(const TestSceneData &data)
    {
        if (!data.asset || data.asset->meshes.empty() || data.asset->meshes.front().primitives.empty())
        {
            return;
        }

        const auto &mesh = data.asset->meshes.front();
        const auto &prim = mesh.primitives.front();
        if (prim.vertices.empty() || prim.indices.empty())
        {
            return;
        }

        ImVec2 screen = ImGui::GetIO().DisplaySize;
        const float padding = 24.0f;
        const float overlaySize = std::min(screen.x, screen.y) * 0.35f;
        if (overlaySize <= 8.0f)
        {
            return;
        }

        ImVec2 viewportPos(screen.x - overlaySize - padding, padding);
        ImVec2 viewportSize(overlaySize, overlaySize);
        ImDrawList *draw = ImGui::GetForegroundDrawList();

        float minX = prim.vertices.front().position[0];
        float minY = prim.vertices.front().position[1];
        float minZ = prim.vertices.front().position[2];
        float maxX = minX;
        float maxY = minY;
        float maxZ = minZ;
        for (const auto &v : prim.vertices)
        {
            minX = std::min(minX, v.position[0]);
            minY = std::min(minY, v.position[1]);
            minZ = std::min(minZ, v.position[2]);
            maxX = std::max(maxX, v.position[0]);
            maxY = std::max(maxY, v.position[1]);
            maxZ = std::max(maxZ, v.position[2]);
        }

        float centerX = 0.5f * (minX + maxX);
        float centerY = 0.5f * (minY + maxY);
        float centerZ = 0.5f * (minZ + maxZ);
        float extentX = maxX - minX;
        float extentY = maxY - minY;
        float extentZ = maxZ - minZ;
        float maxExtent = std::max(extentX, std::max(extentY, extentZ));
        float invScale = maxExtent > 0.0f ? 1.4f / maxExtent : 1.0f;

        float time = static_cast<float>(ImGui::GetTime());
        float angle = time * 0.4f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        auto project = [&](float x, float y, float z) -> ImVec2
        {
            float dx = (x - centerX) * invScale;
            float dy = (y - centerY) * invScale;
            float dz = (z - centerZ) * invScale;

            float rx = dx * cosA - dz * sinA;
            float rz = dx * sinA + dz * cosA;

            float cameraZ = 2.8f;
            float depth = rz + cameraZ;
            float px = rx / depth;
            float py = dy / depth;

            float scale = 0.45f * std::min(viewportSize.x, viewportSize.y);
            return ImVec2(viewportPos.x + viewportSize.x * 0.5f + px * scale,
                          viewportPos.y + viewportSize.y * 0.5f - py * scale);
        };

        const openchordix::assets::MaterialData *material = nullptr;
        if (prim.materialIndex && *prim.materialIndex < data.asset->materials.size())
        {
            material = &data.asset->materials[*prim.materialIndex];
        }

        bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
        if (material && material->baseColorImageIndex && *material->baseColorImageIndex < data.imageTextures.size())
        {
            texture = data.imageTextures[*material->baseColorImageIndex];
        }

        ImTextureRef texRef = ImGui::GetIO().Fonts->TexRef;
        ImVec2 fallbackUv = ImGui::GetIO().Fonts->TexUvWhitePixel;
        bool useVertexUv = false;
        if (bgfx::isValid(texture))
        {
            texRef = ImTextureRef(ImGui::toId(texture, IMGUI_FLAGS_ALPHA_BLEND, 0));
            useVertexUv = true;
        }

        ImVec4 tint(1.0f, 1.0f, 1.0f, 1.0f);
        if (material)
        {
            tint = ImVec4(material->baseColorFactor[0],
                          material->baseColorFactor[1],
                          material->baseColorFactor[2],
                          material->baseColorFactor[3]);
        }
        ImU32 tintColor = ImGui::ColorConvertFloat4ToU32(tint);

        struct ProjectedVertex
        {
            ImVec2 pos;
            ImVec2 uv;
            float depth;
        };

        auto projectVertex = [&](const openchordix::assets::Vertex &v) -> ProjectedVertex
        {
            float dx = (v.position[0] - centerX) * invScale;
            float dy = (v.position[1] - centerY) * invScale;
            float dz = (v.position[2] - centerZ) * invScale;

            float rx = dx * cosA - dz * sinA;
            float rz = dx * sinA + dz * cosA;

            float cameraZ = 2.8f;
            float depth = rz + cameraZ;
            float px = rx / depth;
            float py = dy / depth;

            float scale = 0.45f * std::min(viewportSize.x, viewportSize.y);
            ImVec2 pos(viewportPos.x + viewportSize.x * 0.5f + px * scale,
                       viewportPos.y + viewportSize.y * 0.5f - py * scale);
            ImVec2 uv = useVertexUv ? ImVec2(v.uv[0], v.uv[1]) : fallbackUv;
            return {pos, uv, depth};
        };

        struct Triangle
        {
            ProjectedVertex v0;
            ProjectedVertex v1;
            ProjectedVertex v2;
            float depth;
        };

        const std::size_t maxTriangles = 5000;
        std::size_t triCount = std::min(prim.indices.size() / 3, maxTriangles);
        std::vector<Triangle> tris;
        tris.reserve(triCount);
        for (std::size_t i = 0; i < triCount; ++i)
        {
            std::size_t idx = i * 3;
            const auto &v0 = prim.vertices[prim.indices[idx]];
            const auto &v1 = prim.vertices[prim.indices[idx + 1]];
            const auto &v2 = prim.vertices[prim.indices[idx + 2]];

            Triangle tri{};
            tri.v0 = projectVertex(v0);
            tri.v1 = projectVertex(v1);
            tri.v2 = projectVertex(v2);
            tri.depth = (tri.v0.depth + tri.v1.depth + tri.v2.depth) / 3.0f;
            tris.push_back(tri);
        }

        std::sort(tris.begin(), tris.end(),
                  [](const Triangle &a, const Triangle &b)
                  { return a.depth > b.depth; });

        draw->PushTexture(texRef);
        draw->PrimReserve(static_cast<int>(tris.size() * 3), static_cast<int>(tris.size() * 3));
        for (const auto &tri : tris)
        {
            draw->PrimVtx(tri.v0.pos, tri.v0.uv, tintColor);
            draw->PrimVtx(tri.v1.pos, tri.v1.uv, tintColor);
            draw->PrimVtx(tri.v2.pos, tri.v2.uv, tintColor);
        }
        draw->PopTexture();
    }
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
        return std::make_unique<TestScene>(ui_, testSceneData_);
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
    {
        auto &platformIo = ImGui::GetPlatformIO();
        platformIo.Platform_GetClipboardTextFn = getClipboardText;
        platformIo.Platform_SetClipboardTextFn = setClipboardText;
        platformIo.Platform_ClipboardUserData = gfx_.window();
    }

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
        { return sceneNames(); },
        [&](std::string_view name)
        {
            auto id = sceneIdFromName(name);
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
            destroyModelTextures(testSceneData_.imageTextures);
            std::filesystem::path modelPath(path);
            if (modelPath.is_relative())
            {
                modelPath = std::filesystem::current_path() / modelPath;
            }
            auto result = openchordix::assets::loadGltfAsset(modelPath);
            if (!result.ok())
            {
                testSceneData_.lastError = result.error;
                testSceneData_.asset.reset();
                return false;
            }
            testSceneData_.asset = std::make_shared<openchordix::assets::GltfAsset>(std::move(*result.asset));
            testSceneData_.assetPath = modelPath.string();
            testSceneData_.lastError.clear();
            testSceneData_.imageTextures.clear();
            testSceneData_.imageTextures.reserve(testSceneData_.asset->images.size());
            for (const auto &image : testSceneData_.asset->images)
            {
                if (image.width <= 0 || image.height <= 0 || image.rgba.empty())
                {
                    testSceneData_.imageTextures.push_back(BGFX_INVALID_HANDLE);
                    continue;
                }
                const bgfx::Memory *mem = bgfx::copy(image.rgba.data(), static_cast<uint32_t>(image.rgba.size()));
                auto handle = bgfx::createTexture2D(
                    static_cast<uint16_t>(image.width),
                    static_cast<uint16_t>(image.height),
                    false,
                    1,
                    bgfx::TextureFormat::RGBA8,
                    0,
                    mem);
                testSceneData_.imageTextures.push_back(handle);
            }
            return true;
        },
        [&]()
        {
            destroyModelTextures(testSceneData_.imageTextures);
            testSceneData_.asset.reset();
            testSceneData_.assetPath.clear();
            testSceneData_.lastError.clear();
        },
        [&]()
        {
            if (!testSceneData_.asset)
            {
                if (!testSceneData_.lastError.empty())
                {
                    return std::string("Model error: ") + testSceneData_.lastError;
                }
                return std::string("No model loaded.");
            }
            const auto &asset = *testSceneData_.asset;
            return "Loaded model: " + testSceneData_.assetPath +
                   " | meshes " + std::to_string(asset.meshes.size()) +
                   " | materials " + std::to_string(asset.materials.size()) +
                   " | images " + std::to_string(asset.images.size());
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
        updateImGuiKeyboard(gfx_.window(), input);
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
        drawModelOverlay(testSceneData_);

        devConsole_.render();

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
