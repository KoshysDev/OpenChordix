#pragma once

#include <vector>
#include <optional>
#include <string>
#include <memory>
#include <utility>

#include <rtaudio/RtAudio.h>

#include "AudioManager.h"
#include "AudioConfig.h"
#include "NoteConverter.h"

struct PitchState
{
    float frequency = 0.0f;
    NoteInfo note{};
};

struct DeviceEntry
{
    unsigned int id = 0;
    RtAudio::DeviceInfo info{};
};

class AudioSession
{
public:
    AudioSession(std::vector<int> allowedSampleRates, std::vector<int> allowedBufferSizes);

    void refreshDevices(RtAudio::Api api);
    bool startMonitoring();
    void stopMonitoring(bool clearStatus);
    void updatePitch(NoteConverter &noteConverter);

    const std::vector<DeviceEntry> &devices() const { return devices_; }
    std::optional<unsigned int> selectedInputDevice() const { return selectedInputDevice_; }
    std::optional<unsigned int> selectedOutputDevice() const { return selectedOutputDevice_; }
    void selectInputDevice(unsigned int id);
    void selectOutputDevice(unsigned int id);
    bool trySelectInputDevice(unsigned int id);
    bool trySelectOutputDevice(unsigned int id);
    bool applyConfig(const AudioConfig &config);
    AudioConfig currentConfig() const;
    std::string status() const { return status_; }
    void setStatus(std::string message) { status_ = std::move(message); }
    bool monitoring() const { return monitoring_; }
    unsigned int sampleRate() const { return sampleRate_; }
    unsigned int bufferFrames() const { return bufferFrames_; }
    void setSampleRate(unsigned int sr) { sampleRate_ = sr; }
    void setBufferFrames(unsigned int frames) { bufferFrames_ = frames; }
    RtAudio::Api api() const { return api_; }
    void setApi(RtAudio::Api api) { api_ = api; }
    PitchState pitch() const { return pitch_; }
    const std::vector<int> &allowedSampleRates() const { return allowedSampleRates_; }
    const std::vector<int> &allowedBufferSizes() const { return allowedBufferSizes_; }

private:
    std::unique_ptr<AudioManager> manager_;
    std::vector<DeviceEntry> devices_;
    std::optional<unsigned int> selectedInputDevice_;
    std::optional<unsigned int> selectedOutputDevice_;
    RtAudio::Api api_ = RtAudio::Api::UNSPECIFIED;
    unsigned int sampleRate_ = 48000;
    unsigned int bufferFrames_ = 1024;
    std::string status_;
    bool monitoring_ = false;
    PitchState pitch_{};
    std::vector<int> allowedSampleRates_;
    std::vector<int> allowedBufferSizes_;
};
