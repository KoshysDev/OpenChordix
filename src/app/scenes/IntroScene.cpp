#include "IntroScene.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <system_error>
#include <vector>
#include <stb/stb_image.h>

IntroScene::IntroScene(float durationSec) : duration_(durationSec) {}

IntroScene::~IntroScene()
{
    if (bgfx::isValid(bannerTex_))
    {
        bgfx::destroy(bannerTex_);
    }
}

void IntroScene::render(float dt, const FrameInput & /*input*/, GraphicsContext &gfx, std::atomic<bool> & /*quitFlag*/)
{
    if (finished_)
    {
        return;
    }

    if (!bannerLoaded_)
    {
        bannerLoaded_ = loadBanner();
    }

    elapsed_ += dt;
    const float t = std::min(elapsed_ / duration_, 1.0f);
    float alpha = 1.0f;
    if (elapsed_ < fadeIn_)
    {
        alpha = std::clamp(elapsed_ / fadeIn_, 0.0f, 1.0f);
    }
    else if (elapsed_ > duration_ - fadeOut_)
    {
        alpha = std::clamp((duration_ - elapsed_) / fadeOut_, 0.0f, 1.0f);
    }

    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("Intro", nullptr, flags))
    {
        if (bgfx::isValid(bannerTex_))
        {
            const float margin = 32.0f;
            ImVec2 targetSize = bannerSize_;

            // Scale down if larger than screen with margin
            const float scale = std::min(
                (screen.x - 2.0f * margin) / targetSize.x,
                (screen.y - 2.0f * margin) / targetSize.y);
            if (scale < 1.0f)
            {
                targetSize.x *= scale;
                targetSize.y *= scale;
            }

            ImVec2 pos((screen.x - targetSize.x) * 0.5f, (screen.y - targetSize.y) * 0.5f);
            ImGui::SetCursorPos(pos);
            ImGui::Image(bannerTex_, targetSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, alpha));
        }
        else
        {
            ImGui::SetCursorPos(ImVec2(screen.x * 0.5f - 120.0f, screen.y * 0.5f - 20.0f));
            ImVec4 color = ImVec4(0.90f, 0.95f, 1.0f, (0.2f + 0.8f * t) * alpha);
            ImGui::TextColored(color, "OpenChordix");
        }
    }
    ImGui::End();

    if (elapsed_ >= duration_)
    {
        finished_ = true;
    }
}

bool IntroScene::loadBanner()
{
    const auto path = resolveBannerPath();
    if (path.empty())
    {
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc *data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data)
    {
        return false;
    }

    const bgfx::Memory *mem = bgfx::copy(data, static_cast<uint32_t>(width * height * 4));
    stbi_image_free(data);

    bannerTex_ = bgfx::createTexture2D(
        static_cast<uint16_t>(width),
        static_cast<uint16_t>(height),
        false,
        1,
        bgfx::TextureFormat::RGBA8,
        0,
        mem);

    if (bgfx::isValid(bannerTex_))
    {
        bannerSize_ = ImVec2(static_cast<float>(width), static_cast<float>(height));
        return true;
    }

    return false;
}

std::filesystem::path IntroScene::resolveBannerPath() const
{
    std::vector<std::filesystem::path> candidates;
#ifdef OPENCHORDIX_BANNER_PATH
    candidates.emplace_back(std::filesystem::path{OPENCHORDIX_BANNER_PATH});
#endif

    const char *names[] = {"banner.png", "Banner.png", "banner.webp"};
    const auto cwd = std::filesystem::current_path();

    std::filesystem::path exeDir;
#if defined(__linux__)
    std::error_code ec;
    auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec)
    {
        exeDir = exe.parent_path();
    }
#endif
    if (exeDir.empty())
    {
        exeDir = cwd;
    }

    for (const auto *name : names)
    {
        candidates.emplace_back(cwd / "assets/icons" / name);
        candidates.emplace_back(cwd / name);
        candidates.emplace_back(exeDir / "assets/icons" / name);
    }

    for (const auto &path : candidates)
    {
        std::error_code ec;
        if (!path.empty() && std::filesystem::exists(path, ec))
        {
            return path;
        }
    }

    return {};
}
