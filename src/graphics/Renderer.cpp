#include "Renderer.h"

#include <bgfx/platform.h>
#include <iostream>

namespace openchordix
{

    Renderer::Renderer() = default;

    Renderer::~Renderer()
    {
        shutdown();
    }

    bool Renderer::initialize(const RendererConfig &config)
    {
        if (initialized_)
        {
            return true;
        }

        config_ = config;

        bgfx::Init init{};
        init.type = config_.type;
        init.vendorId = BGFX_PCI_ID_NONE;
        init.resolution.width = config_.width;
        init.resolution.height = config_.height;
        init.resolution.reset = config_.resetFlags;
        init.platformData.nwh = config_.nativeWindowHandle;
        init.platformData.ndt = config_.nativeDisplayType;
        init.platformData.context = config_.nativeContext;
        init.platformData.backBuffer = config_.nativeBackBuffer;
        init.platformData.backBufferDS = config_.nativeBackBufferDepth;
        init.platformData.type = config_.nativeHandleType;

        if (config_.headless)
        {
            init.type = bgfx::RendererType::Direct3D12;
            init.resolution.width = 0;
            init.resolution.height = 0;
            init.resolution.reset = BGFX_RESET_NONE;
        }

        if (!bgfx::init(init))
        {
            std::cerr << "Renderer: Failed to initialize bgfx." << std::endl;
            return false;
        }

        uint32_t debugFlags = 0;
        if (config_.enableDebugText)
        {
            debugFlags |= BGFX_DEBUG_TEXT;
        }
        if (config_.enableStats)
        {
            debugFlags |= BGFX_DEBUG_STATS;
        }
        bgfx::setDebug(debugFlags);

        if (!config_.headless)
        {
            bgfx::setViewClear(
                config_.viewId,
                BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                config_.clearColor,
                1.0f,
                0);

            bgfx::setViewRect(
                config_.viewId,
                0,
                0,
                static_cast<uint16_t>(config_.width),
                static_cast<uint16_t>(config_.height));
        }

        initialized_ = true;
        return true;
    }

    void Renderer::shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        bgfx::shutdown();
        initialized_ = false;
    }

    void Renderer::beginFrame(uint16_t viewId) const
    {
        if (!initialized_)
        {
            return;
        }

        bgfx::touch(viewId);
    }

    void Renderer::endFrame() const
    {
        if (!initialized_)
        {
            return;
        }

        bgfx::frame();
    }

    void Renderer::frame() const
    {
        if (!initialized_)
        {
            return;
        }

        if (!config_.headless)
        {
            bgfx::setViewRect(
                config_.viewId,
                0,
                0,
                static_cast<uint16_t>(config_.width),
                static_cast<uint16_t>(config_.height));
        }

        beginFrame(config_.viewId);
        endFrame();
    }

    void Renderer::resize(uint32_t width, uint32_t height)
    {
        if (!initialized_)
        {
            return;
        }

        config_.width = width;
        config_.height = height;

        if (!config_.headless)
        {
            bgfx::reset(width, height, config_.resetFlags);
            bgfx::setViewRect(
                config_.viewId,
                0,
                0,
                static_cast<uint16_t>(config_.width),
                static_cast<uint16_t>(config_.height));
        }
    }

}
