#pragma once

#include "Scene.h"
#include <bgfx/bgfx.h>
#include <filesystem>
#include <imgui/imgui.h>

class IntroScene : public Scene
{
public:
    explicit IntroScene(float durationSec = 3.0f);
    ~IntroScene();
    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    bool loadBanner();
    std::filesystem::path resolveBannerPath() const;

    float duration_;
    float elapsed_ = 0.0f;
    float fadeIn_ = 0.6f;
    float fadeOut_ = 0.6f;
    bool finished_ = false;
    bool bannerLoaded_ = false;
    bgfx::TextureHandle bannerTex_ = BGFX_INVALID_HANDLE;
    ImVec2 bannerSize_{0.0f, 0.0f};
};
