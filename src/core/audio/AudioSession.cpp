#include "audio/AudioSession.h"

#include <algorithm>
#include <cmath>

namespace
{
    unsigned int nearestValue(const std::vector<unsigned int> &options, unsigned int target)
    {
        auto it = std::min_element(options.begin(), options.end(), [target](unsigned int lhs, unsigned int rhs)
                                   {
                                       long long dl = std::llabs(static_cast<long long>(lhs) - static_cast<long long>(target));
                                       long long dr = std::llabs(static_cast<long long>(rhs) - static_cast<long long>(target));
                                       return dl < dr;
                                   });
        return it != options.end() ? *it : 0u;
    }
}

AudioSession::AudioSession(std::vector<int> allowedSampleRates, std::vector<int> allowedBufferSizes)
    : allowedSampleRates_(std::move(allowedSampleRates)), allowedBufferSizes_(std::move(allowedBufferSizes))
{
    if (!allowedSampleRates_.empty())
    {
        sampleRate_ = static_cast<unsigned int>(allowedSampleRates_.front());
    }
    if (!allowedBufferSizes_.empty())
    {
        bufferFrames_ = static_cast<unsigned int>(allowedBufferSizes_.front());
    }
}

void AudioSession::refreshDevices(RtAudio::Api api)
{
    stopMonitoring(false);
    devices_.clear();
    selectedInputDevice_.reset();
    selectedOutputDevice_.reset();
    status_.clear();
    api_ = api;

    try
    {
        manager_ = std::make_unique<AudioManager>(api_);
    }
    catch (const std::exception &e)
    {
        status_ = std::string("Audio initialization failed: ") + e.what();
        manager_.reset();
        return;
    }

    for (unsigned int id : manager_->getDeviceIds())
    {
        devices_.push_back(DeviceEntry{id, manager_->getDeviceInfo(id)});
    }

    unsigned int defaultInput = manager_->getDefaultInputDeviceId();
    unsigned int defaultOutput = manager_->getDefaultOutputDeviceId();

    auto preferDefaultInput = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                                           { return entry.id == defaultInput && entry.info.inputChannels > 0; });
    if (preferDefaultInput != devices_.end())
    {
        selectedInputDevice_ = preferDefaultInput->id;
    }
    else
    {
        auto firstInput = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                                       { return entry.info.inputChannels > 0; });
        if (firstInput != devices_.end())
        {
            selectedInputDevice_ = firstInput->id;
        }
    }

    auto preferDefaultOutput = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                                            { return entry.id == defaultOutput && entry.info.outputChannels > 0; });
    if (preferDefaultOutput != devices_.end())
    {
        selectedOutputDevice_ = preferDefaultOutput->id;
    }
    else
    {
        auto firstOutput = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                                        { return entry.info.outputChannels > 0; });
        if (firstOutput != devices_.end())
        {
            selectedOutputDevice_ = firstOutput->id;
        }
    }

    autoDetectPreferredStreamSettings();

    if (devices_.empty())
    {
        status_ = "No audio devices found for this API.";
    }
    else if (!selectedInputDevice_.has_value() && !selectedOutputDevice_.has_value())
    {
        status_ = "No input- or output-capable device detected.";
    }
    else if (!selectedInputDevice_.has_value())
    {
        status_ = "No input-capable device detected.";
    }
    else if (!selectedOutputDevice_.has_value())
    {
        status_ = "No output-capable device detected.";
    }
    else
    {
        RtAudio::DeviceInfo inputInfo = manager_->getDeviceInfo(*selectedInputDevice_);
        RtAudio::DeviceInfo outputInfo = manager_->getDeviceInfo(*selectedOutputDevice_);
        status_ = "Ready. Input: " + inputInfo.name + " / Output: " + outputInfo.name;
    }
}

bool AudioSession::startMonitoring()
{
    if (!manager_)
    {
        status_ = "Audio stack is not ready.";
        return false;
    }
    if (!selectedInputDevice_.has_value())
    {
        status_ = "Pick an input device first.";
        return false;
    }

    if (!selectedOutputDevice_.has_value())
    {
        status_ = "Pick an output device first.";
        return false;
    }

    RtAudio::DeviceInfo inputInfo = manager_->getDeviceInfo(*selectedInputDevice_);
    RtAudio::DeviceInfo outputInfo = manager_->getDeviceInfo(*selectedOutputDevice_);
    if (inputInfo.inputChannels == 0)
    {
        status_ = "Selected device has no input channels.";
        return false;
    }
    if (outputInfo.outputChannels == 0)
    {
        status_ = "Selected output has no output channels.";
        return false;
    }

    unsigned int requestedBuffer = bufferFrames_;
    if (manager_->getCurrentApi() == RtAudio::Api::UNIX_JACK)
    {
        requestedBuffer = 0;
    }

    if (!manager_->openMonitoringStream(*selectedInputDevice_, *selectedOutputDevice_, sampleRate_, requestedBuffer))
    {
        status_ = "Failed to open the audio stream.";
        return false;
    }
    if (!manager_->startStream())
    {
        status_ = "Audio stream refused to start.";
        manager_->closeStream();
        return false;
    }

    status_ = "Monitoring " + inputInfo.name + " -> " + outputInfo.name;
    monitoring_ = true;
    return true;
}

void AudioSession::stopMonitoring(bool clearStatus)
{
    if (manager_)
    {
        manager_->stopStream();
        manager_->closeStream();
    }
    monitoring_ = false;
    if (clearStatus)
    {
        status_.clear();
    }
}

void AudioSession::updatePitch(NoteConverter &noteConverter)
{
    if (!manager_ || !manager_->isStreamRunning())
    {
        monitoring_ = false;
        pitch_ = {};
        return;
    }

    float current_freq = manager_->getLatestPitchHz();
    if (current_freq > 10.0f)
    {
        pitch_.frequency = current_freq;
        pitch_.note = noteConverter.getNoteInfo(current_freq);
        monitoring_ = true;
    }
    else
    {
        monitoring_ = manager_->isStreamRunning();
    }
}

void AudioSession::selectInputDevice(unsigned int id, bool autoDetectSettings)
{
    selectedInputDevice_ = id;
    if (autoDetectSettings)
    {
        autoDetectPreferredStreamSettings();
    }
}

void AudioSession::selectOutputDevice(unsigned int id, bool autoDetectSettings)
{
    selectedOutputDevice_ = id;
    if (autoDetectSettings)
    {
        autoDetectPreferredStreamSettings();
    }
}

bool AudioSession::trySelectInputDevice(unsigned int id, bool autoDetectSettings)
{
    auto it = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                           { return entry.id == id && entry.info.inputChannels > 0; });
    if (it != devices_.end())
    {
        selectInputDevice(id, autoDetectSettings);
        return true;
    }
    return false;
}

bool AudioSession::trySelectOutputDevice(unsigned int id, bool autoDetectSettings)
{
    auto it = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                           { return entry.id == id && entry.info.outputChannels > 0; });
    if (it != devices_.end())
    {
        selectOutputDevice(id, autoDetectSettings);
        return true;
    }
    return false;
}

bool AudioSession::applyConfig(const AudioConfig &config)
{
    bool inputOk = config.inputDeviceId != 0 && trySelectInputDevice(config.inputDeviceId, false);
    bool outputOk = config.outputDeviceId != 0 && trySelectOutputDevice(config.outputDeviceId, false);

    if (inputOk && outputOk)
    {
        const DeviceEntry *input = findDevice(*selectedInputDevice_);
        const DeviceEntry *output = findDevice(*selectedOutputDevice_);
        if (input && output)
        {
            unsigned int detectedRate = choosePreferredSampleRate(input->info, output->info);
            unsigned int detectedBuffer = choosePreferredBufferFrames(detectedRate);
            sampleRate_ = config.sampleRate > 0 ? config.sampleRate : detectedRate;
            bufferFrames_ = config.bufferFrames > 0 ? config.bufferFrames : detectedBuffer;
        }
    }

    return inputOk && outputOk && config.isUsable();
}

AudioConfig AudioSession::currentConfig() const
{
    AudioConfig config{};
    config.api = api_;
    config.inputDeviceId = selectedInputDevice_.value_or(0);
    config.outputDeviceId = selectedOutputDevice_.value_or(0);
    config.sampleRate = sampleRate_;
    config.bufferFrames = bufferFrames_;
    return config;
}

const DeviceEntry *AudioSession::findDevice(unsigned int id) const
{
    auto it = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                           { return entry.id == id; });
    return it != devices_.end() ? &(*it) : nullptr;
}

void AudioSession::autoDetectPreferredStreamSettings()
{
    if (!selectedInputDevice_.has_value() || !selectedOutputDevice_.has_value())
    {
        return;
    }

    const DeviceEntry *input = findDevice(*selectedInputDevice_);
    const DeviceEntry *output = findDevice(*selectedOutputDevice_);
    if (!input || !output)
    {
        return;
    }

    sampleRate_ = choosePreferredSampleRate(input->info, output->info);
    bufferFrames_ = choosePreferredBufferFrames(sampleRate_);
}

unsigned int AudioSession::choosePreferredSampleRate(const RtAudio::DeviceInfo &inputInfo, const RtAudio::DeviceInfo &outputInfo) const
{
    if (allowedSampleRates_.empty())
    {
        return sampleRate_;
    }

    auto supportsRate = [](const RtAudio::DeviceInfo &info, unsigned int rate)
    {
        if (info.sampleRates.empty())
        {
            return true;
        }
        return std::find(info.sampleRates.begin(), info.sampleRates.end(), rate) != info.sampleRates.end();
    };

    std::vector<unsigned int> allAllowed;
    allAllowed.reserve(allowedSampleRates_.size());
    std::vector<unsigned int> compatible;
    compatible.reserve(allowedSampleRates_.size());

    for (int rate : allowedSampleRates_)
    {
        if (rate <= 0)
        {
            continue;
        }
        unsigned int r = static_cast<unsigned int>(rate);
        allAllowed.push_back(r);
        if (supportsRate(inputInfo, r) && supportsRate(outputInfo, r))
        {
            compatible.push_back(r);
        }
    }

    const std::vector<unsigned int> &pool = compatible.empty() ? allAllowed : compatible;
    if (pool.empty())
    {
        return sampleRate_;
    }

    auto contains = [](const std::vector<unsigned int> &values, unsigned int target)
    {
        return std::find(values.begin(), values.end(), target) != values.end();
    };

    if (outputInfo.preferredSampleRate > 0)
    {
        return nearestValue(pool, outputInfo.preferredSampleRate);
    }
    if (inputInfo.preferredSampleRate > 0)
    {
        return nearestValue(pool, inputInfo.preferredSampleRate);
    }
    if (contains(pool, 48000u))
    {
        return 48000u;
    }
    if (contains(pool, 44100u))
    {
        return 44100u;
    }
    return pool.front();
}

unsigned int AudioSession::choosePreferredBufferFrames(unsigned int sampleRate) const
{
    if (allowedBufferSizes_.empty())
    {
        return bufferFrames_;
    }

    std::vector<unsigned int> allAllowed;
    allAllowed.reserve(allowedBufferSizes_.size());
    for (int frames : allowedBufferSizes_)
    {
        if (frames > 0)
        {
            allAllowed.push_back(static_cast<unsigned int>(frames));
        }
    }

    if (allAllowed.empty())
    {
        return bufferFrames_;
    }

    unsigned int preferred = sampleRate > 48000u ? 512u : 256u;
    return nearestValue(allAllowed, preferred);
}
