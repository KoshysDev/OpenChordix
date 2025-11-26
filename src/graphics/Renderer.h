#pragma once

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <cstdint>

namespace openchordix
{

    struct RendererConfig
    {
        uint32_t width = 1280;
        uint32_t height = 720;
        bgfx::RendererType::Enum type = bgfx::RendererType::Count;
        uint32_t resetFlags = BGFX_RESET_VSYNC;
        uint16_t viewId = 0;
        uint32_t clearColor = 0x303030ff;
        bool enableDebugText = true;
        bool enableStats = false;

        // Native handles; for X11 these are Display* (ndt) and Window (nwh).
        void *nativeDisplayType = nullptr;
        void *nativeWindowHandle = nullptr;
        void *nativeContext = nullptr;
        void *nativeBackBuffer = nullptr;
        void *nativeBackBufferDepth = nullptr;
        bgfx::NativeWindowHandleType::Enum nativeHandleType = bgfx::NativeWindowHandleType::Count;

        bool headless = true;
    };

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        bool initialize(const RendererConfig &config = RendererConfig{});
        void shutdown();

        void beginFrame(uint16_t viewId = 0) const;
        void endFrame() const;
        void frame() const;
        void resize(uint32_t width, uint32_t height);

        bool isInitialized() const { return initialized_; }
        const RendererConfig &config() const { return config_; }

    private:
        bool initialized_ = false;
        RendererConfig config_{};
    };

}
