#include "GraphicsContext.h"

#include <iostream>
#include <array>
#include <system_error>
#include <vector>
#include <optional>

#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <imgui/imgui.h>

#if defined(GLFW_EXPOSE_NATIVE_X11)
#include <X11/Xlib.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace
{
    std::filesystem::path executableDir()
    {
#ifdef _WIN32
        wchar_t buffer[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
        if (length == 0 || length == std::size(buffer))
        {
            return {};
        }
        return std::filesystem::path(buffer).parent_path();
#else
        std::error_code ec;
        const auto exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (ec)
        {
            return {};
        }
        return exePath.parent_path();
#endif
    }

    void scrollCallback(GLFWwindow *window, double /*xoffset*/, double yoffset)
    {
        auto *ctx = static_cast<GraphicsContext *>(glfwGetWindowUserPointer(window));
        if (ctx)
        {
            ctx->addScrollDelta(static_cast<float>(yoffset));
        }
    }
}

GraphicsContext::GraphicsContext()
{
    rendererConfig_.clearColor = 0x11141cff;
}

GraphicsContext::~GraphicsContext()
{
    shutdown();
}

bool GraphicsContext::initializeWindowed(const char *title)
{
    const bool hasWayland = std::getenv("WAYLAND_DISPLAY") != nullptr;
    const bool hasX11 = std::getenv("DISPLAY") != nullptr;
    if (hasWayland && hasX11)
    {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

    if (!glfwInit())
    {
        std::cerr << "GLFW init failed.\n";
        return false;
    }

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
    int width = mode ? mode->width : 1280;
    int height = mode ? mode->height : 720;

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_)
    {
        return false;
    }

    if (const auto iconPath = findIconPath(); !iconPath.empty())
    {
        setWindowIcon(iconPath);
    }

    setWindowClassHint();

    if (monitor && mode)
    {
        glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetScrollCallback(window_, scrollCallback);
    startedWithWindow_ = true;

    glfwGetFramebufferSize(window_, &lastFbWidth_, &lastFbHeight_);
    rendererConfig_.width = static_cast<uint32_t>(lastFbWidth_);
    rendererConfig_.height = static_cast<uint32_t>(lastFbHeight_);
    rendererConfig_.headless = false;
    rendererConfig_.type = bgfx::RendererType::Count;

    updateNativeHandles();
    return true;
}

void GraphicsContext::updateNativeHandles()
{
#if defined(GLFW_EXPOSE_NATIVE_WAYLAND)
    void *wlDisplay = glfwGetWaylandDisplay();
    void *wlSurface = glfwGetWaylandWindow(window_);
    if (wlDisplay && wlSurface)
    {
        rendererConfig_.headless = false;
        rendererConfig_.type = bgfx::RendererType::Vulkan;
        rendererConfig_.nativeDisplayType = wlDisplay;
        rendererConfig_.nativeWindowHandle = wlSurface;
        rendererConfig_.nativeHandleType = bgfx::NativeWindowHandleType::Wayland;
        return;
    }
#endif
#if defined(GLFW_EXPOSE_NATIVE_X11)
    Display *xDisplay = glfwGetX11Display();
    Window xWindow = glfwGetX11Window(window_);
    if (xDisplay && xWindow)
    {
        rendererConfig_.headless = false;
        rendererConfig_.type = bgfx::RendererType::Count;
        rendererConfig_.nativeDisplayType = xDisplay;
        rendererConfig_.nativeWindowHandle = (void *)(uintptr_t)xWindow;
        rendererConfig_.nativeHandleType = bgfx::NativeWindowHandleType::Default;
    }
#endif
}

std::filesystem::path GraphicsContext::findIconPath() const
{
    const char *iconNames[] = {"AppIcon.png", "AppIcon.ico"};
    std::vector<std::filesystem::path> candidates;

    const auto exeDir = executableDir();
    if (!exeDir.empty())
    {
        for (const auto *name : iconNames)
        {
            candidates.emplace_back(exeDir / name);
            candidates.emplace_back(exeDir / "assets/icons" / name);
        }
    }

    for (const auto *name : iconNames)
    {
        candidates.emplace_back(std::filesystem::current_path() / name);
        candidates.emplace_back(std::filesystem::current_path() / "assets/icons" / name);
    }

#ifdef OPENCHORDIX_APP_ICON_PNG_PATH
    candidates.emplace_back(std::filesystem::path{OPENCHORDIX_APP_ICON_PNG_PATH});
#endif
#ifdef OPENCHORDIX_APP_ICON_ICO_PATH
    candidates.emplace_back(std::filesystem::path{OPENCHORDIX_APP_ICON_ICO_PATH});
#endif

    for (const auto &candidate : candidates)
    {
        std::error_code ec;
        if (!candidate.empty() && std::filesystem::exists(candidate, ec))
        {
            return candidate;
        }
    }

    return {};
}

bool GraphicsContext::setWindowIcon(const std::filesystem::path &iconPath)
{
    if (!window_)
    {
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc *pixels = stbi_load(iconPath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        std::cerr << "Failed to load icon from " << iconPath << std::endl;
        return false;
    }

    GLFWimage image{};
    image.width = width;
    image.height = height;
    image.pixels = pixels;

    glfwSetWindowIcon(window_, 1, &image);
    stbi_image_free(pixels);
    return true;
}

void GraphicsContext::setWindowClassHint()
{
#if defined(GLFW_EXPOSE_NATIVE_X11)
    Display *display = glfwGetX11Display();
    Window xWindow = glfwGetX11Window(window_);
    if (display && xWindow)
    {
        XClassHint classHint;
        classHint.res_name = const_cast<char *>("OpenChordix");
        classHint.res_class = const_cast<char *>("OpenChordix");
        XSetClassHint(display, xWindow, &classHint);
    }
#endif
}

bool GraphicsContext::initializeRenderer()
{
    if (!startedWithWindow_)
    {
        return false;
    }

    if (!renderer_.initialize(rendererConfig_))
    {
        std::cerr << "Renderer: Failed to initialize bgfx." << std::endl;
        return false;
    }
    return true;
}

FrameInput GraphicsContext::pollFrame()
{
    FrameInput input{};
    if (!window_)
    {
        return input;
    }

    glfwPollEvents();
    glfwGetFramebufferSize(window_, &lastFbWidth_, &lastFbHeight_);
    if (renderer_.isInitialized() && (rendererConfig_.width != (uint32_t)lastFbWidth_ || rendererConfig_.height != (uint32_t)lastFbHeight_))
    {
        renderer_.resize(static_cast<uint32_t>(lastFbWidth_), static_cast<uint32_t>(lastFbHeight_));
        rendererConfig_.width = static_cast<uint32_t>(lastFbWidth_);
        rendererConfig_.height = static_cast<uint32_t>(lastFbHeight_);
    }

    double mx = 0.0;
    double my = 0.0;
    glfwGetCursorPos(window_, &mx, &my);
    input.mouseX = static_cast<int32_t>(mx);
    input.mouseY = static_cast<int32_t>(my);

    if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        input.buttonMask |= IMGUI_MBUT_LEFT;
    if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        input.buttonMask |= IMGUI_MBUT_RIGHT;
    if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
        input.buttonMask |= IMGUI_MBUT_MIDDLE;

    input.scroll = static_cast<int32_t>(scrollDelta_);
    scrollDelta_ = 0.0f;
    return input;
}

bool GraphicsContext::shouldClose() const
{
    return !window_ || glfwWindowShouldClose(window_);
}

void GraphicsContext::shutdown()
{
    if (renderer_.isInitialized())
    {
        renderer_.shutdown();
    }
    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}
