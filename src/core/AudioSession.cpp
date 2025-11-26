#include "AudioSession.h"

#include <algorithm>

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

void AudioSession::selectInputDevice(unsigned int id)
{
    selectedInputDevice_ = id;
}

void AudioSession::selectOutputDevice(unsigned int id)
{
    selectedOutputDevice_ = id;
}

bool AudioSession::trySelectInputDevice(unsigned int id)
{
    auto it = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                           { return entry.id == id && entry.info.inputChannels > 0; });
    if (it != devices_.end())
    {
        selectInputDevice(id);
        return true;
    }
    return false;
}

bool AudioSession::trySelectOutputDevice(unsigned int id)
{
    auto it = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                           { return entry.id == id && entry.info.outputChannels > 0; });
    if (it != devices_.end())
    {
        selectOutputDevice(id);
        return true;
    }
    return false;
}

bool AudioSession::applyConfig(const AudioConfig &config)
{
    if (config.sampleRate > 0)
    {
        setSampleRate(config.sampleRate);
    }
    if (config.bufferFrames > 0)
    {
        setBufferFrames(config.bufferFrames);
    }

    bool inputOk = config.inputDeviceId != 0 && trySelectInputDevice(config.inputDeviceId);
    bool outputOk = config.outputDeviceId != 0 && trySelectOutputDevice(config.outputDeviceId);

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
