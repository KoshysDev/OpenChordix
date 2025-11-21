#pragma once

#include <cstdint>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "Renderer.h"

struct FrameInput
{
    int32_t mouseX = 0;
    int32_t mouseY = 0;
    uint8_t buttonMask = 0;
    int32_t scroll = 0;
};

class GraphicsContext
{
public:
    GraphicsContext();
    ~GraphicsContext();

    bool initializeWindowed(const char *title);
    bool initializeRenderer();
    void shutdown();

    FrameInput pollFrame();
    void addScrollDelta(float delta) { scrollDelta_ += delta; }
    bool shouldClose() const;

    GLFWwindow *window() const { return window_; }
    const openchordix::RendererConfig &config() const { return rendererConfig_; }
    openchordix::Renderer &renderer() { return renderer_; }

private:
    void updateNativeHandles();

    GLFWwindow *window_{nullptr};
    openchordix::Renderer renderer_{};
    openchordix::RendererConfig rendererConfig_{};
    bool startedWithWindow_{false};
    float scrollDelta_{0.0f};
    int lastFbWidth_{0};
    int lastFbHeight_{0};
};
