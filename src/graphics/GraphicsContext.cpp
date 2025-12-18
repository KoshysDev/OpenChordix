#include "GraphicsContext.h"

#include <iostream>
#include <system_error>
#include <vector>
#include <algorithm>

#include "DisplayManager.h"

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

#ifdef _WIN32
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((void *)-4)
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ((void *)-3)
#endif

    void enableHighDpiAwareness()
    {
        if (HMODULE user32 = LoadLibraryW(L"user32.dll"))
        {
            using SetDpiAwarenessContextFn = BOOL(WINAPI *)(void *);
            auto setDpiAwarenessContext = reinterpret_cast<SetDpiAwarenessContextFn>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
            if (setDpiAwarenessContext)
            {
                if (setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ||
                    setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
                {
                    FreeLibrary(user32);
                    return;
                }
            }
            FreeLibrary(user32);
        }

        if (HMODULE shcore = LoadLibraryW(L"Shcore.dll"))
        {
            using SetProcessDpiAwarenessFn = HRESULT(WINAPI *)(int);
            auto setProcessDpiAwareness = reinterpret_cast<SetProcessDpiAwarenessFn>(
                GetProcAddress(shcore, "SetProcessDpiAwareness"));
            if (setProcessDpiAwareness && SUCCEEDED(setProcessDpiAwareness(2)))
            {
                FreeLibrary(shcore);
                return;
            }
            FreeLibrary(shcore);
        }

        SetProcessDPIAware();
    }
#else
    void enableHighDpiAwareness() {}
#endif
}

GraphicsContext::GraphicsContext()
{
    rendererConfig_.clearColor = 0x11141cff;
}

GraphicsContext::~GraphicsContext()
{
    shutdown();
}

void GraphicsContext::syncFramebufferSize(int fallbackWidth, int fallbackHeight)
{
    if (!window_)
    {
        return;
    }

    glfwGetFramebufferSize(window_, &lastFbWidth_, &lastFbHeight_);
    if (fallbackWidth > 0)
    {
        lastFbWidth_ = std::max(lastFbWidth_, fallbackWidth);
    }
    if (fallbackHeight > 0)
    {
        lastFbHeight_ = std::max(lastFbHeight_, fallbackHeight);
    }
    rendererConfig_.width = static_cast<uint32_t>(lastFbWidth_);
    rendererConfig_.height = static_cast<uint32_t>(lastFbHeight_);
}

GLFWmonitor *GraphicsContext::currentMonitor() const
{
    if (!window_)
    {
        return nullptr;
    }

    GLFWmonitor *attached = glfwGetWindowMonitor(window_);
    if (attached)
    {
        return attached;
    }

    return openchordix::DisplayManager::monitorForWindow(window_).handle;
}

namespace
{
    void logGlfwError(const char *context)
    {
        const char *desc = nullptr;
        int code = glfwGetError(&desc);
        if (code != GLFW_NO_ERROR)
        {
            std::cerr << "[GLFW] " << context << " failed (code " << code << "): "
                      << (desc ? desc : "unknown error") << std::endl;
        }
    }
}

bool GraphicsContext::initializeWindowed(const char *title)
{
    enableHighDpiAwareness();

    const bool hasWayland = std::getenv("WAYLAND_DISPLAY") != nullptr;
    const bool hasX11 = std::getenv("DISPLAY") != nullptr;

    if (hasWayland)
    {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    }
    else if (hasX11)
    {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    }

    if (!glfwInit())
    {
        std::cerr << "GLFW init failed.\n";
        logGlfwError("glfwInit");
        return false;
    }

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
    std::cout << "[Graphics] Monitors detected: " << monitorCount << std::endl;
    for (int i = 0; i < monitorCount; ++i)
    {
        const char *name = glfwGetMonitorName(monitors[i]);
        const GLFWvidmode *m = glfwGetVideoMode(monitors[i]);
        int wx = 0;
        int wy = 0;
        int ww = 0;
        int wh = 0;
        glfwGetMonitorWorkarea(monitors[i], &wx, &wy, &ww, &wh);
        std::cout << "  [" << i << "] " << (name ? name : "Unknown") << " mode "
                  << (m ? std::to_string(m->width) + "x" + std::to_string(m->height) : "n/a")
                  << " work " << ww << "x" << wh << " at (" << wx << "," << wy << ")" << std::endl;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    openchordix::MonitorInfo monitorInfo = openchordix::DisplayManager::bestMonitor();
    GLFWmonitor *monitor = monitorInfo.handle;

    const GLFWvidmode *mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
    int monitorX = monitorInfo.x;
    int monitorY = monitorInfo.y;

    int width = mode ? mode->width : 1920;
    int height = mode ? mode->height : 1080;

    // Create a borderless window and place it on the chosen monitor
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!window_)
    {
        logGlfwError("glfwCreateWindow");
        return false;
    }

    if (const auto iconPath = findIconPath(); !iconPath.empty())
    {
        setWindowIcon(iconPath);
    }

    setWindowClassHint();

    if (monitor && mode)
    {
        glfwSetWindowAttrib(window_, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(window_, monitor, 0, 0, width, height, mode->refreshRate);
    }
    else
    {
        glfwSetWindowAttrib(window_, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowPos(window_, monitorX, monitorY);
        glfwSetWindowSize(window_, width, height);
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetScrollCallback(window_, scrollCallback);
    startedWithWindow_ = true;

    syncFramebufferSize(width, height);
    rendererConfig_.headless = false;
    rendererConfig_.type = bgfx::RendererType::Count;

    updateNativeHandles();

    int winX = 0;
    int winY = 0;
    int winW = 0;
    int winH = 0;
    int fbW = 0;
    int fbH = 0;
    glfwGetWindowPos(window_, &winX, &winY);
    glfwGetWindowSize(window_, &winW, &winH);
    glfwGetFramebufferSize(window_, &fbW, &fbH);
    std::cout << "[Graphics] Created window at (" << winX << "," << winY << ") size " << winW << "x" << winH
              << " framebuffer " << fbW << "x" << fbH << std::endl;
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

void GraphicsContext::applyResize(uint32_t width, uint32_t height)
{
    uint32_t targetW = width;
    uint32_t targetH = height;

    if (window_)
    {
        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window_, &fbW, &fbH);
        if (fbW > 0 && fbH > 0)
        {
            targetW = std::max(targetW, static_cast<uint32_t>(fbW));
            targetH = std::max(targetH, static_cast<uint32_t>(fbH));
        }
    }

    if (rendererConfig_.width == targetW && rendererConfig_.height == targetH && renderer_.isInitialized())
    {
        return;
    }

    rendererConfig_.width = targetW;
    rendererConfig_.height = targetH;
    lastFbWidth_ = static_cast<int>(targetW);
    lastFbHeight_ = static_cast<int>(targetH);

    if (renderer_.isInitialized())
    {
        renderer_.resize(targetW, targetH);
    }
}
