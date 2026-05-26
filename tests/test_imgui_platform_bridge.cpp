#include <catch2/catch_test_macros.hpp>

#include <type_traits>

#include "platform/ImGuiPlatformBridge.h"

namespace
{
static_assert(std::is_constructible_v<ImGuiPlatformBridge, GLFWwindow *>);
static_assert(std::is_same_v<
              decltype(&ImGuiPlatformBridge::installClipboard),
              void (ImGuiPlatformBridge::*)() const>);
static_assert(std::is_same_v<
              decltype(&ImGuiPlatformBridge::updateKeyboard),
              void (ImGuiPlatformBridge::*)(const FrameInput &) const>);
}

TEST_CASE("ImGuiPlatformBridge exposes the expected platform API surface", "[platform]")
{
    CHECK(std::is_constructible_v<ImGuiPlatformBridge, GLFWwindow *>);
}
