#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <rtaudio/RtAudio.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct AudioConfig {
    bool configured = false;
    int apiValue = static_cast<int>(RtAudio::Api::UNSPECIFIED);
    unsigned int inputDeviceId = 0;
    unsigned int outputDeviceId = 0;
    unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 1024;

    // JSON serialization
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AudioConfig, configured, apiValue, inputDeviceId, sampleRate, bufferFrames);
};

class ConfigManager {
public:
    ConfigManager(const std::string& configFileName = "settings.json");

    bool loadConfig();
    bool saveConfig() const;

    AudioConfig& getAudioConfig();
    const AudioConfig& getAudioConfig() const;

    bool isFirstLaunch() const;

private:
    std::string m_configFilePath;
    AudioConfig m_audioConfig;
};

#endif