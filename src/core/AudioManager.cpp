#include "AudioManager.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <cmath>

// --- Static Error Callback Implementation ---
void AudioManager::defaultErrorCallback(RtAudioErrorType type, const std::string &errorText)
{
    if (type == RTAUDIO_WARNING)
    {
        std::cerr << "RtAudio Warning: " << errorText << std::endl;
    }
    else if (type != RTAUDIO_NO_ERROR)
    {
        std::cerr << "RtAudio Error (" << type << "): " << errorText << std::endl;
    }
}

// --- Constructor Implementation ---
AudioManager::AudioManager(RtAudio::Api api) : selectedApi_(api),
                                               actualApi_(RtAudio::Api::UNSPECIFIED)
{
    std::cout << "Attempting to initialize RtAudio with requested API: "
              << RtAudio::getApiDisplayName(selectedApi_) << " (" << selectedApi_ << ")" << std::endl;

    try
    {
        audio_ = std::make_unique<RtAudio>(selectedApi_, &AudioManager::defaultErrorCallback);
        actualApi_ = audio_->getCurrentApi();

        if (selectedApi_ != RtAudio::Api::UNSPECIFIED && actualApi_ != selectedApi_)
        {
            std::cerr << "Warning: Requested API (" << RtAudio::getApiDisplayName(selectedApi_)
                      << ") was not available or chosen. Using API: "
                      << RtAudio::getApiDisplayName(actualApi_) << std::endl;
        }
        std::cout << "RtAudio initialized successfully using API: "
                  << RtAudio::getApiDisplayName(actualApi_) << std::endl;
    }
    catch (const std::exception &e)
    {
        AudioManager::defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during RtAudio instantiation: " + std::string(e.what()));
        throw std::runtime_error("Failed to initialize RtAudio instance.");
    }

    if (audio_ && audio_->getDeviceCount() == 0 && actualApi_ != RtAudio::Api::RTAUDIO_DUMMY)
    {
        std::cerr << "Warning: RtAudio initialized, but no devices were found for the selected API.\n";
    }

    if (!audio_)
    {
        AudioManager::defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "RtAudio unique_ptr is null after construction attempt.");
        throw std::runtime_error("RtAudio unique_ptr is null after construction.");
    }
}

// --- Static Method: Get Available APIs ---
std::vector<RtAudio::Api> AudioManager::getAvailableApis()
{
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    return apis;
}

std::vector<unsigned int> AudioManager::getDeviceIds() const
{
    if (!audio_)
    {
        AudioManager::defaultErrorCallback(RTAUDIO_INVALID_USE, "getDeviceIds called on uninitialized AudioManager (null audio pointer).");
        return {};
    }

    try
    {
        return audio_->getDeviceIds();
    }
    catch (const std::exception &e)
    {
        AudioManager::defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during getDeviceIds(): " + std::string(e.what()));
        return {};
    }
}

// --- Device Listing Method Implementation ---
bool AudioManager::listDevices() const
{
    if (!audio_)
    {
        AudioManager::defaultErrorCallback(RTAUDIO_INVALID_USE, "listDevices called on uninitialized AudioManager (null audio pointer).");
        return false;
    }

    unsigned int deviceCount = 0;
    try
    {
        deviceCount = audio_->getDeviceCount();
    }
    catch (const std::exception &e)
    {
        AudioManager::defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during getDeviceCount(): " + std::string(e.what()));
        return false;
    }

    std::cout << "\nFound " << deviceCount << " audio devices for API "
              << RtAudio::getApiDisplayName(actualApi_) << ":" << std::endl;

    if (deviceCount < 1)
    {
        std::cerr << "No audio devices found for this API.\n";
        return false;
    }

    std::vector<unsigned int> deviceIds = audio_->getDeviceIds();
    bool devices_listed = false;

    for (unsigned int id : deviceIds)
    {
        try
        {
            RtAudio::DeviceInfo info = audio_->getDeviceInfo(id);

            std::cout << "  Device ID " << id << ": " << info.name;
            if (info.isDefaultInput)
                std::cout << " (DEFAULT INPUT)";
            if (info.isDefaultOutput)
                std::cout << " (DEFAULT OUTPUT)";
            std::cout << std::endl;
            std::cout << "    Output Channels: " << info.outputChannels << std::endl;
            std::cout << "    Input Channels: " << info.inputChannels << std::endl;
            std::cout << "    Sample Rates: ";
            if (info.sampleRates.empty())
            {
                std::cout << "(None reported)";
            }
            else
            {
                for (unsigned int rate : info.sampleRates)
                {
                    std::cout << rate << " ";
                }
            }
            std::cout << std::endl;
            devices_listed = true;
        }
        catch (const std::exception &e)
        {
            // Catch exceptions specifically from getDeviceInfo
            AudioManager::defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception getting info for device " + std::to_string(id) + ": " + e.what());
            // Continue trying to list other devices
        }
    }
    std::cout << std::endl;

    // Print default device IDs reported by the current API context
    std::cout << "Default Output Device ID (for this API): " << getDefaultOutputDeviceId() << std::endl;
    std::cout << "Default Input Device ID (for this API): " << getDefaultInputDeviceId() << std::endl;

    return devices_listed;
}

// --- Implementation for getDeviceInfo ---
RtAudio::DeviceInfo AudioManager::getDeviceInfo(unsigned int deviceId) const
{
    if (!audio_)
    {
        throw std::runtime_error("AudioManager not initialized (getDeviceInfo).");
    }
    try
    {
        return audio_->getDeviceInfo(deviceId);
    }
    catch (...)
    { // Catch potential RtAudio errors
        defaultErrorCallback(RTAUDIO_INVALID_DEVICE, "Failed to get device info for ID: " + std::to_string(deviceId));
        // Return an empty/default DeviceInfo struct to indicate failure
        return RtAudio::DeviceInfo{}; // Default constructor initializes members appropriately
    }
}

// --- Static Audio Callback Implementation ---
int AudioManager::monitoringCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
                                     double streamTime, RtAudioStreamStatus status, void *userData)
{
    (void)streamTime;
    AudioCallbackData *cbData = static_cast<AudioCallbackData *>(userData);
    PitchDetector *detector = (cbData) ? cbData->pitchDetector : nullptr;

    if (!cbData || !detector)
    { // Check both
        std::cerr << "Error: Callback user data or PitchDetector missing!" << std::endl;
        return 2;
    }
    unsigned int inputChannels = cbData->inputChannels;
    unsigned int outputChannels = cbData->outputChannels;

    if (status & RTAUDIO_INPUT_OVERFLOW)
        std::cerr << "Input overflow!" << std::endl;
    if (status & RTAUDIO_OUTPUT_UNDERFLOW)
        std::cerr << "Output underflow!" << std::endl;

    float *rt_in_buffer = static_cast<float *>(inputBuffer);
    float *rt_out_buffer = static_cast<float *>(outputBuffer);

    // --- Pitch Detection ---
    if (rt_in_buffer != nullptr)
    {
        detector->process(rt_in_buffer, nFrames, inputChannels);
    }

    // --- Monitoring Output ---
    if (rt_out_buffer != nullptr && rt_in_buffer != nullptr)
    {
        if (inputChannels == 1 && outputChannels == 2)
        {
            for (unsigned int i = 0; i < nFrames; ++i)
            {
                rt_out_buffer[i * 2 + 0] = rt_in_buffer[i];
                rt_out_buffer[i * 2 + 1] = rt_in_buffer[i];
            }
        }
        else if (inputChannels == outputChannels)
        {
            memcpy(rt_out_buffer, rt_in_buffer, nFrames * inputChannels * sizeof(float));
        }
        else if (inputChannels == 2 && outputChannels == 1)
        {
            for (unsigned int i = 0; i < nFrames; ++i)
            {
                rt_out_buffer[i] = (rt_in_buffer[i * 2 + 0] + rt_in_buffer[i * 2 + 1]) * 0.5f;
            }
        }
        else
        {
            memset(rt_out_buffer, 0, nFrames * outputChannels * sizeof(float));
        }
    }
    else if (rt_out_buffer != nullptr)
    {
        memset(rt_out_buffer, 0, nFrames * outputChannels * sizeof(float));
    }

    return 0;
}

// --- Stream Management Implementations ---
bool AudioManager::openMonitoringStream(unsigned int inputDeviceId, unsigned int sampleRate, unsigned int bufferFrames)
{
    if (!audio_)
    {
        defaultErrorCallback(RTAUDIO_INVALID_USE, "Cannot open stream, AudioManager not initialized.");
        return false;
    }
    if (streamIsOpen_)
    {
        defaultErrorCallback(RTAUDIO_INVALID_USE, "Cannot open stream, another stream is already open.");
        return false;
    }

    // --- Get Device Info ---
    RtAudio::DeviceInfo inputInfo;
    RtAudio::DeviceInfo outputInfo;
    unsigned int outputDeviceId = 0;
    try
    {
        inputInfo = getDeviceInfo(inputDeviceId);
        outputDeviceId = getDefaultOutputDeviceId();
        if (outputDeviceId == 0)
            throw std::runtime_error("No default output device found.");
        outputInfo = getDeviceInfo(outputDeviceId);
    }
    catch (const std::runtime_error &e)
    {
        defaultErrorCallback(RTAUDIO_INVALID_DEVICE, "Failed to get device info for stream setup: " + std::string(e.what()));
        return false;
    }
    if (inputInfo.inputChannels == 0)
    {
        defaultErrorCallback(RTAUDIO_INVALID_PARAMETER, "Selected input device (ID: " + std::to_string(inputDeviceId) + ") has no input channels.");
        return false;
    }
    if (outputInfo.outputChannels == 0)
    {
        defaultErrorCallback(RTAUDIO_INVALID_PARAMETER, "Default output device (ID: " + std::to_string(outputDeviceId) + ") has no output channels.");
        return false;
    }

    // --- Determine RtAudio Stream Channel Counts ---
    streamOutputChannels_ = (outputInfo.outputChannels >= 2) ? 2 : 1;
    streamInputChannels_ = 1; // Request 1 channel for RtAudio input
    std::cout << "Requesting " << streamOutputChannels_ << " output channel(s)." << std::endl;
    std::cout << "Requesting " << streamInputChannels_ << " input channel(s) from RtAudio." << std::endl;

    // --- Set RtAudio Stream Parameters ---
    RtAudio::StreamParameters iParams;
    iParams.deviceId = inputDeviceId;
    iParams.nChannels = streamInputChannels_;
    iParams.firstChannel = 0;
    RtAudio::StreamParameters oParams;
    oParams.deviceId = outputDeviceId;
    oParams.nChannels = streamOutputChannels_;
    oParams.firstChannel = 0;

    // --- Store Stream Settings ---
    streamSampleRate_ = sampleRate;
    // Store requested size, actual size will be updated by openStream
    unsigned int requestedBufferFrames = bufferFrames;
    unsigned int actualBufferFrames = requestedBufferFrames;

    // --- Reset Pitch Detector ---
    pitch_detector_.reset();

    // --- Prepare Callback Data ---
    callbackData_.inputChannels = streamInputChannels_;
    callbackData_.outputChannels = streamOutputChannels_;
    callbackData_.pitchDetector = nullptr;

    // --- Open the RtAudio Stream ---
    std::cout << "Attempting to open RtAudio stream: SR=" << streamSampleRate_ << " Buf=" << requestedBufferFrames
              << " Input Ch=" << streamInputChannels_ << " Output Ch=" << streamOutputChannels_ << std::endl;
    RtAudioErrorType result = RTAUDIO_NO_ERROR;
    try
    {
        result = audio_->openStream(&oParams, &iParams, RTAUDIO_FLOAT32, streamSampleRate_,
                                    &actualBufferFrames, // Pass address!
                                    &AudioManager::monitoringCallback, &callbackData_, nullptr);
    }
    catch (const std::exception &e)
    {
        defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during openStream: " + std::string(e.what()));
        streamIsOpen_ = false;
        return false;
    }

    if (result != RTAUDIO_NO_ERROR)
    {
        std::cerr << "RtAudio openStream failed with code: " << result << std::endl;
        streamIsOpen_ = false;
        return false;
    }

    // --- RtAudio Stream Opened Successfully ---
    streamBufferFrames_ = actualBufferFrames;
    streamIsOpen_ = true;
    std::cout << "RtAudio Stream opened successfully. Actual buffer size: " << streamBufferFrames_ << std::endl;

    try
    {
        // Calculate hop size
        unsigned int hopSize = streamBufferFrames_;
        if (hopSize == 0)
            hopSize = 1;

        pitch_detector_ = std::make_unique<PitchDetector>(streamBufferFrames_, hopSize, streamSampleRate_);
        callbackData_.pitchDetector = pitch_detector_.get();
        std::cout << "PitchDetector initialized successfully." << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error initializing PitchDetector: " << e.what() << std::endl;
        closeStream();
        return false;
    }

    return true;
}

bool AudioManager::startStream()
{
    if (!audio_ || !streamIsOpen_)
    {
        defaultErrorCallback(RTAUDIO_INVALID_USE, "Cannot start stream, stream not open.");
        return false;
    }
    if (streamIsRunning_)
    {
        defaultErrorCallback(RTAUDIO_WARNING, "Stream already running.");
        return true;
    }

    RtAudioErrorType result = RTAUDIO_NO_ERROR;
    try
    {
        result = audio_->startStream();
    }
    catch (const std::exception &e)
    {
        defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during startStream: " + std::string(e.what()));
        streamIsRunning_ = false;
        return false;
    }

    if (result != RTAUDIO_NO_ERROR)
    {
        std::cerr << "RtAudio startStream failed with code: " << result << std::endl;
        streamIsRunning_ = false;
    }
    else
    {
        std::cout << "Stream started successfully." << std::endl;
        streamIsRunning_ = true;
    }
    return streamIsRunning_;
}

bool AudioManager::stopStream()
{
    if (!audio_ || !streamIsOpen_)
        return true;
    if (!streamIsRunning_)
        return true;

    RtAudioErrorType result = RTAUDIO_NO_ERROR;
    std::cout << "Attempting to stop stream..." << std::endl;
    try
    {
        result = audio_->stopStream();
    }
    catch (const std::exception &e)
    {
        defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during stopStream: " + std::string(e.what()));
        streamIsRunning_ = false;
        return false;
    }

    if (result != RTAUDIO_NO_ERROR)
    {
        std::cerr << "RtAudio stopStream failed with code: " << result << std::endl;
        streamIsRunning_ = false;
    }
    else
    {
        std::cout << "Stream stopped successfully." << std::endl;
        streamIsRunning_ = false;
    }
    return !streamIsRunning_;
}

void AudioManager::closeStream()
{
    if (!audio_ && !pitch_detector_)
        return;

    // Check RtAudio stream state first
    bool wasStreamOpen = false;
    try
    {
        if (audio_ && audio_->isStreamOpen())
        {
            wasStreamOpen = true;
            if (audio_->isStreamRunning())
            {
                std::cout << "Stream is running, stopping it before closing..." << std::endl;
                audio_->stopStream();
            }
            std::cout << "Closing RtAudio stream..." << std::endl;
            audio_->closeStream();
        }
    }
    catch (const std::exception &e)
    {
        defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during RtAudio stop/closeStream: " + std::string(e.what()));
    }

    // Destroy the pitch detector after the stream is closed or confirmed closed
    pitch_detector_.reset();
    if (wasStreamOpen)
        std::cout << "PitchDetector destroyed." << std::endl;

    // Reset internal state flags
    streamIsOpen_ = false;
    streamIsRunning_ = false;
    if (wasStreamOpen)
        std::cout << "Stream resources released." << std::endl;
}

float AudioManager::getLatestPitchHz() const
{
    return pitch_detector_ ? pitch_detector_->getPitchHz() : 0.0f;
}

bool AudioManager::isStreamOpen() const
{
    return streamIsOpen_ && audio_ && audio_->isStreamOpen(); // Check internal flag and RtAudio's state
}

bool AudioManager::isStreamRunning() const
{
    return streamIsRunning_ && audio_ && audio_->isStreamRunning(); // Check internal flag and RtAudio's state
}

RtAudio::Api AudioManager::getCurrentApi() const
{
    return audio_ ? actualApi_ : RtAudio::Api::UNSPECIFIED;
}

unsigned int AudioManager::getDefaultInputDeviceId() const
{
    return audio_ ? audio_->getDefaultInputDevice() : 0;
}

unsigned int AudioManager::getDefaultOutputDeviceId() const
{
    return audio_ ? audio_->getDefaultOutputDevice() : 0;
}
