#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <filesystem>
#include <vector>

#include "Renderer.h"

struct FrameInput
{
    int32_t mouseX = 0;
    int32_t mouseY = 0;
    uint8_t buttonMask = 0;
    int32_t scroll = 0;
    std::vector<uint32_t> inputChars;
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
    void applyResize(uint32_t width, uint32_t height);
    void syncFramebufferSize(int fallbackWidth = 0, int fallbackHeight = 0);
    bool shouldClose() const;
    void pushInputChar(uint32_t codepoint);

    GLFWwindow *window() const { return window_; }
    GLFWmonitor *currentMonitor() const;
    const openchordix::RendererConfig &config() const { return rendererConfig_; }
    openchordix::Renderer &renderer() { return renderer_; }

private:
    void updateNativeHandles();
    void setWindowClassHint();
    std::filesystem::path findIconPath() const;
    bool setWindowIconFromEmbedded();
    bool setWindowIcon(const std::filesystem::path &iconPath);

    GLFWwindow *window_{nullptr};
    openchordix::Renderer renderer_{};
    openchordix::RendererConfig rendererConfig_{};
    bool startedWithWindow_{false};
    float scrollDelta_{0.0f};
    std::vector<uint32_t> inputChars_{};
    int lastFbWidth_{0};
    int lastFbHeight_{0};
};
