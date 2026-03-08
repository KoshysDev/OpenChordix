#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace openchordix::track
{
    std::optional<std::string> readAudioDuration(const std::filesystem::path &audioPath);
}

