#pragma once

#include <GLFW/glfw3.h>

#include "GraphicsContext.h"

struct ImGuiContext;

class ImGuiPlatformBridge
{
public:
    explicit ImGuiPlatformBridge(GLFWwindow *window);

    void installClipboard() const;
    void updateKeyboard(const FrameInput &input) const;

private:
    static const char *getClipboardText(ImGuiContext *ctx);
    static void setClipboardText(ImGuiContext *ctx, const char *text);

    GLFWwindow *window_ = nullptr;
};
