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
    selectedDevice_.reset();
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
    auto preferDefault = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                                      { return entry.id == defaultInput && entry.info.inputChannels > 0; });
    if (preferDefault != devices_.end())
    {
        selectedDevice_ = preferDefault->id;
    }
    else
    {
        auto firstInput = std::find_if(devices_.begin(), devices_.end(), [&](const DeviceEntry &entry)
                                       { return entry.info.inputChannels > 0; });
        if (firstInput != devices_.end())
        {
            selectedDevice_ = firstInput->id;
        }
    }

    if (devices_.empty())
    {
        status_ = "No audio devices found for this API.";
    }
    else if (!selectedDevice_.has_value())
    {
        status_ = "No input-capable device detected.";
    }
    else
    {
        RtAudio::DeviceInfo info = manager_->getDeviceInfo(*selectedDevice_);
        status_ = "Ready. Default input: " + info.name;
    }
}

bool AudioSession::startMonitoring()
{
    if (!manager_)
    {
        status_ = "Audio stack is not ready.";
        return false;
    }
    if (!selectedDevice_.has_value())
    {
        status_ = "Pick an input device first.";
        return false;
    }

    RtAudio::DeviceInfo info = manager_->getDeviceInfo(*selectedDevice_);
    if (info.inputChannels == 0)
    {
        status_ = "Selected device has no input channels.";
        return false;
    }

    unsigned int requestedBuffer = bufferFrames_;
    if (manager_->getCurrentApi() == RtAudio::Api::UNIX_JACK)
    {
        requestedBuffer = 0;
    }

    if (!manager_->openMonitoringStream(*selectedDevice_, sampleRate_, requestedBuffer))
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

    status_ = "Monitoring " + info.name;
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

void AudioSession::selectDevice(unsigned int id)
{
    selectedDevice_ = id;
}
