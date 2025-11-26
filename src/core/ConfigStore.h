#pragma once

#include <optional>
#include <filesystem>

#include "AudioConfig.h"

class ConfigStore
{
public:
    ConfigStore();

    std::optional<AudioConfig> loadAudioConfig() const;
    bool saveAudioConfig(const AudioConfig &config) const;

    std::filesystem::path audioConfigPath() const { return audioConfigPath_; }

private:
    std::filesystem::path audioConfigPath_;
};
