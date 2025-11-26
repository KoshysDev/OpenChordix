#include "ConfigStore.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace
{
    std::filesystem::path resolveExecutableDirectory()
    {
#if defined(__linux__)
        std::error_code ec;
        auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec)
        {
            return exe.parent_path();
        }
#endif
        return std::filesystem::current_path();
    }
}

ConfigStore::ConfigStore() : audioConfigPath_(resolveExecutableDirectory() / "audio.conf") {}

std::optional<AudioConfig> ConfigStore::loadAudioConfig() const
{
    std::ifstream in(audioConfigPath_);
    if (!in.good())
    {
        return std::nullopt;
    }

    AudioConfig config{};
    std::string line;
    while (std::getline(in, line))
    {
        auto pos = line.find('=');
        if (pos == std::string::npos)
        {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        std::istringstream iss(value);
        if (key == "api")
        {
            int apiValue = static_cast<int>(config.api);
            if (iss >> apiValue)
            {
                config.api = static_cast<RtAudio::Api>(apiValue);
            }
        }
        else if (key == "input_device")
        {
            unsigned int inputId = config.inputDeviceId;
            if (iss >> inputId)
            {
                config.inputDeviceId = inputId;
            }
        }
        else if (key == "output_device")
        {
            unsigned int outputId = config.outputDeviceId;
            if (iss >> outputId)
            {
                config.outputDeviceId = outputId;
            }
        }
        else if (key == "sample_rate")
        {
            unsigned int sr = config.sampleRate;
            if (iss >> sr)
            {
                config.sampleRate = sr;
            }
        }
        else if (key == "buffer_frames")
        {
            unsigned int bf = config.bufferFrames;
            if (iss >> bf)
            {
                config.bufferFrames = bf;
            }
        }
    }

    if (!config.isUsable())
    {
        return std::nullopt;
    }

    return config;
}

bool ConfigStore::saveAudioConfig(const AudioConfig &config) const
{
    if (!config.isUsable())
    {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(audioConfigPath_.parent_path(), ec);

    std::ofstream out(audioConfigPath_, std::ios::trunc);
    if (!out.good())
    {
        return false;
    }

    out << "api=" << static_cast<int>(config.api) << '\n';
    out << "input_device=" << config.inputDeviceId << '\n';
    out << "output_device=" << config.outputDeviceId << '\n';
    out << "sample_rate=" << config.sampleRate << '\n';
    out << "buffer_frames=" << config.bufferFrames << '\n';

    return true;
}
