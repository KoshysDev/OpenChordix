#include "ImGuiPlatformBridge.h"

#include <imgui/imgui.h>

ImGuiPlatformBridge::ImGuiPlatformBridge(GLFWwindow *window)
    : window_(window)
{
}

void ImGuiPlatformBridge::installClipboard() const
{
    auto &platformIo = ImGui::GetPlatformIO();
    platformIo.Platform_GetClipboardTextFn = getClipboardText;
    platformIo.Platform_SetClipboardTextFn = setClipboardText;
    platformIo.Platform_ClipboardUserData = window_;
}

void ImGuiPlatformBridge::updateKeyboard(const FrameInput &input) const
{
    ImGuiIO &io = ImGui::GetIO();

    io.AddKeyEvent(ImGuiMod_Shift, glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiMod_Ctrl, glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiMod_Alt, glfwGetKey(window_, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiMod_Super, glfwGetKey(window_, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS);

    io.AddKeyEvent(ImGuiKey_Tab, glfwGetKey(window_, GLFW_KEY_TAB) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_LeftArrow, glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_RightArrow, glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_UpArrow, glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_DownArrow, glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_PageUp, glfwGetKey(window_, GLFW_KEY_PAGE_UP) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_PageDown, glfwGetKey(window_, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Home, glfwGetKey(window_, GLFW_KEY_HOME) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_End, glfwGetKey(window_, GLFW_KEY_END) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Insert, glfwGetKey(window_, GLFW_KEY_INSERT) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Delete, glfwGetKey(window_, GLFW_KEY_DELETE) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Backspace, glfwGetKey(window_, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Space, glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Enter, glfwGetKey(window_, GLFW_KEY_ENTER) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_KeypadEnter, glfwGetKey(window_, GLFW_KEY_KP_ENTER) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Escape, glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_A, glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_C, glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_V, glfwGetKey(window_, GLFW_KEY_V) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_X, glfwGetKey(window_, GLFW_KEY_X) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Y, glfwGetKey(window_, GLFW_KEY_Y) == GLFW_PRESS);
    io.AddKeyEvent(ImGuiKey_Z, glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS);

    for (uint32_t codepoint : input.inputChars)
    {
        io.AddInputCharacter(codepoint);
    }
}

const char *ImGuiPlatformBridge::getClipboardText(ImGuiContext * /*ctx*/)
{
    auto *window = static_cast<GLFWwindow *>(ImGui::GetPlatformIO().Platform_ClipboardUserData);
    return window ? glfwGetClipboardString(window) : nullptr;
}

void ImGuiPlatformBridge::setClipboardText(ImGuiContext * /*ctx*/, const char *text)
{
    auto *window = static_cast<GLFWwindow *>(ImGui::GetPlatformIO().Platform_ClipboardUserData);
    if (window)
    {
        glfwSetClipboardString(window, text ? text : "");
    }
}
