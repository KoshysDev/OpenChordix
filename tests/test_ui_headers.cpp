#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "ui/DeviceSelector.h"
#include "ui/FileDialog.h"

namespace
{
using DeviceOptionsFn = std::vector<DeviceOption> (*)(const std::vector<DeviceEntry> &, DeviceRole, std::optional<unsigned int>);
using DeviceComboFn = bool (*)(const char *, const std::vector<DeviceOption> &, DeviceRole, const std::function<void(unsigned int)> &);
using DeviceListFn = bool (*)(const char *, const std::vector<DeviceOption> &, DeviceRole, const std::function<void(unsigned int)> &, ImVec2);
using FileDialogDrawFn = std::optional<std::filesystem::path> (FileDialog::*)(const ImVec2 &);

static_assert(std::is_enum_v<DeviceRole>);
static_assert(std::is_same_v<decltype(&channelCount), int (*)(const RtAudio::DeviceInfo &, DeviceRole)>);
static_assert(std::is_same_v<decltype(&isUsable), bool (*)(const RtAudio::DeviceInfo &, DeviceRole)>);
static_assert(std::is_same_v<decltype(&DeviceSelector::makeOptions), DeviceOptionsFn>);
static_assert(std::is_same_v<decltype(&DeviceSelector::combo), DeviceComboFn>);
static_assert(std::is_same_v<decltype(&DeviceSelector::list), DeviceListFn>);
static_assert(std::is_constructible_v<FileDialog, std::string, std::string>);
static_assert(std::is_same_v<decltype(&FileDialog::draw), FileDialogDrawFn>);
}

TEST_CASE("UI headers expose stable selector and dialog contracts", "[ui]")
{
    CHECK(std::is_constructible_v<FileDialog, std::string, std::string>);
    CHECK(std::is_enum_v<DeviceRole>);
}
