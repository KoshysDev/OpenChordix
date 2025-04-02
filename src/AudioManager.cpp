#include "AudioManager.h" 
#include <iostream>       
#include <stdexcept>
#include <cstring>

// --- Static Error Callback Implementation ---
void AudioManager::defaultErrorCallback( RtAudioErrorType type, const std::string &errorText )
{
    if ( type == RTAUDIO_WARNING ) {
        std::cerr << "RtAudio Warning: " << errorText << std::endl;
    } else if ( type != RTAUDIO_NO_ERROR ) {
        std::cerr << "RtAudio Error (" << type << "): " << errorText << std::endl;
        // Potentially set a flag or throw an exception for critical errors if needed
    }
}


// --- Constructor Implementation ---
AudioManager::AudioManager(RtAudio::Api api)
  : selectedApi_(api), // Store the requested API
    actualApi_(RtAudio::Api::UNSPECIFIED) // Initialize actual API to unspecified
{
    std::cout << "Attempting to initialize RtAudio with requested API: "
              << RtAudio::getApiDisplayName(selectedApi_) << " (" << selectedApi_ << ")" << std::endl;

    try {
        // Pass the static error callback to the RtAudio constructor
        audio_ = std::make_unique<RtAudio>(selectedApi_, &AudioManager::defaultErrorCallback);

        // Store the API that RtAudio actually ended up using
        actualApi_ = audio_->getCurrentApi();

        if (selectedApi_ != RtAudio::Api::UNSPECIFIED && actualApi_ != selectedApi_) {
             std::cerr << "Warning: Requested API (" << RtAudio::getApiDisplayName(selectedApi_)
                       << ") was not available or chosen. Using API: "
                       << RtAudio::getApiDisplayName(actualApi_) << std::endl;
        }
         std::cout << "RtAudio initialized successfully using API: "
                   << RtAudio::getApiDisplayName(actualApi_) << std::endl;

    } catch (const std::exception& e) {
        // Catch potential low-level exceptions during RtAudio object creation
        AudioManager::defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during RtAudio instantiation: " + std::string(e.what()));
        // Throw a more specific exception or handle construction failure
        throw std::runtime_error("Failed to initialize RtAudio instance.");
    }

    // Check for devices after successful construction
    if (audio_ && audio_->getDeviceCount() == 0 && actualApi_ != RtAudio::Api::RTAUDIO_DUMMY) {
        std::cerr << "Warning: RtAudio initialized, but no devices were found for the selected API.\n";
    }

    // Check if the audio_ pointer is valid (it should be unless make_unique failed badly)
     if (!audio_) {
          AudioManager::defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "RtAudio unique_ptr is null after construction attempt.");
          throw std::runtime_error("RtAudio unique_ptr is null after construction.");
     }
}


// --- Static Method: Get Available APIs ---
std::vector<RtAudio::Api> AudioManager::getAvailableApis() {
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    return apis;
}


// --- Device Listing Method Implementation ---
bool AudioManager::listDevices() const {
    // Ensure RtAudio object was created successfully
    if (!audio_) {
         AudioManager::defaultErrorCallback(RTAUDIO_INVALID_USE, "listDevices called on uninitialized AudioManager (null audio pointer).");
        return false;
    }

    unsigned int deviceCount = 0;
    try {
         deviceCount = audio_->getDeviceCount(); // This can potentially throw if RtAudio state is bad
    } catch (const std::exception& e) {
         AudioManager::defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during getDeviceCount(): " + std::string(e.what()));
         return false; // Cannot proceed if getting device count fails
    }


    std::cout << "\nFound " << deviceCount << " audio devices for API "
              << RtAudio::getApiDisplayName(actualApi_) << ":" << std::endl;

    if (deviceCount < 1) {
        std::cerr << "No audio devices found for this API.\n";
        return false; // Indicate no devices found
    }

    std::vector<unsigned int> deviceIds = audio_->getDeviceIds();
    bool devices_listed = false;

    for (unsigned int id : deviceIds) {
        try {
            RtAudio::DeviceInfo info = audio_->getDeviceInfo(id);

            std::cout << "  Device ID " << id << ": " << info.name;
            if (info.isDefaultInput) std::cout << " (DEFAULT INPUT)";
            if (info.isDefaultOutput) std::cout << " (DEFAULT OUTPUT)";
            std::cout << std::endl;
            std::cout << "    Output Channels: " << info.outputChannels << std::endl;
            std::cout << "    Input Channels: " << info.inputChannels << std::endl;
             std::cout << "    Sample Rates: ";
             if (info.sampleRates.empty()) {
                 std::cout << "(None reported)";
             } else {
                 for (unsigned int rate : info.sampleRates) {
                     std::cout << rate << " ";
                 }
             }
             std::cout << std::endl;
             devices_listed = true; // successfully listed info for at least one device

        } catch (const std::exception& e) {
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
RtAudio::DeviceInfo AudioManager::getDeviceInfo(unsigned int deviceId) const {
    if (!audio_) {
        throw std::runtime_error("AudioManager not initialized (getDeviceInfo).");
    }
    try {
        return audio_->getDeviceInfo(deviceId);
    } catch (...) { // Catch potential RtAudio errors
        defaultErrorCallback(RTAUDIO_INVALID_DEVICE, "Failed to get device info for ID: " + std::to_string(deviceId));
        // Return an empty/default DeviceInfo struct to indicate failure
        return RtAudio::DeviceInfo{}; // Default constructor initializes members appropriately
    }
}


// --- Static Audio Callback Implementation ---
int AudioManager::monitoringCallback( void *outputBuffer, void *inputBuffer, unsigned int nFrames,
                                       double streamTime, RtAudioStreamStatus status, void *userData )
{
    (void)streamTime; // Prevent unused parameter warning

    // Retrieve context data (like channel count)
    AudioCallbackData* cbData = static_cast<AudioCallbackData*>(userData);
    if (!cbData) {
         // This shouldn't happen if openStream sets userData correctly
         std::cerr << "Error: Callback user data is null!" << std::endl;
         return 2; // Abort stream
    }
    unsigned int channels = cbData->channels;

    if ( status & RTAUDIO_INPUT_OVERFLOW ) {
        std::cerr << "Stream Warning: Input overflow detected!" << std::endl;
    }
    if ( status & RTAUDIO_OUTPUT_UNDERFLOW ) {
        std::cerr << "Stream Warning: Output underflow detected!" << std::endl;
    }

    // Simple Monitoring: Copy input buffer directly to output buffer
    if (inputBuffer != nullptr && outputBuffer != nullptr) {
        // Assuming RTAUDIO_FLOAT32 format for now
        // Size = number of frames * number of channels * size of one sample
        memcpy(outputBuffer, inputBuffer, nFrames * channels * sizeof(float));
    } else {
        // If input-only or output-only, one might be null. For duplex, both should be valid.
         if (inputBuffer == nullptr) std::cerr << "Warning: inputBuffer is null in callback." << std::endl;
         if (outputBuffer == nullptr) std::cerr << "Warning: outputBuffer is null in callback." << std::endl;
         // Optionally zero out output buffer if input is null?
         // if (outputBuffer != nullptr) memset(outputBuffer, 0, nFrames * channels * sizeof(float));
    }

    return 0; // Continue streaming
}


// --- Stream Management Implementations ---

bool AudioManager::openMonitoringStream(unsigned int inputDeviceId, unsigned int sampleRate, unsigned int bufferFrames) {
    if (!audio_) {
        defaultErrorCallback(RTAUDIO_INVALID_USE, "Cannot open stream, AudioManager not initialized.");
        return false;
    }
    if (streamIsOpen_) {
        defaultErrorCallback(RTAUDIO_INVALID_USE, "Cannot open stream, another stream is already open.");
        return false;
    }

    RtAudio::DeviceInfo inputInfo;
    RtAudio::DeviceInfo outputInfo;
    unsigned int outputDeviceId = 0;

    try {
        inputInfo = getDeviceInfo(inputDeviceId);
        outputDeviceId = getDefaultOutputDeviceId();
        if (outputDeviceId == 0) { // 0 is invalid ID
             throw std::runtime_error("No default output device found.");
        }
        outputInfo = getDeviceInfo(outputDeviceId);
    } catch (const std::runtime_error& e) {
         defaultErrorCallback(RTAUDIO_INVALID_DEVICE, "Failed to get device info for stream setup: " + std::string(e.what()));
         return false;
    }


    if (inputInfo.inputChannels == 0) {
        defaultErrorCallback(RTAUDIO_INVALID_PARAMETER, "Selected input device (ID: " + std::to_string(inputDeviceId) + ") has no input channels.");
        return false;
    }
     if (outputInfo.outputChannels == 0) {
        defaultErrorCallback(RTAUDIO_INVALID_PARAMETER, "Default output device (ID: " + std::to_string(outputDeviceId) + ") has no output channels.");
        return false;
    }

    // Determine number of channels (use stereo if available on both, else mono)
    streamChannels_ = std::min(inputInfo.inputChannels, outputInfo.outputChannels);
    if (streamChannels_ > 2) streamChannels_ = 2; // Limit to stereo for simplicity for now
    if (streamChannels_ == 0) { // Should be caught above, but double-check
         defaultErrorCallback(RTAUDIO_INVALID_PARAMETER, "Cannot establish common channel count between input and output devices.");
         return false;
    }
    std::cout << "Using " << streamChannels_ << " channel(s) for monitoring." << std::endl;


    RtAudio::StreamParameters iParams;
    iParams.deviceId = inputDeviceId;
    iParams.nChannels = streamChannels_;
    iParams.firstChannel = 0;

    RtAudio::StreamParameters oParams;
    oParams.deviceId = outputDeviceId;
    oParams.nChannels = streamChannels_;
    oParams.firstChannel = 0;

    // Store parameters needed by callback
    callbackData_.channels = streamChannels_;

    streamSampleRate_ = sampleRate;
    streamBufferFrames_ = bufferFrames; // Store requested buffer size

    RtAudio::StreamOptions options;
    // options.flags |= RTAUDIO_MINIMIZE_LATENCY; // Optional: try to reduce latency

    std::cout << "Attempting to open stream: SR=" << streamSampleRate_ << " Buf=" << streamBufferFrames_ << std::endl;
    RtAudioErrorType result = RTAUDIO_NO_ERROR;
    try {
        // Pass address of our callback data struct as userData
        result = audio_->openStream(&oParams, &iParams, RTAUDIO_FLOAT32, streamSampleRate_,
                                   &streamBufferFrames_, &AudioManager::monitoringCallback, &callbackData_, &options);
    } catch (const std::exception& e) {
        // Catch potential exceptions from within RtAudio openStream (less common)
         defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during openStream: " + std::string(e.what()));
         streamIsOpen_ = false; // Ensure state is correct
         return false;
    }


    if (result != RTAUDIO_NO_ERROR) {
        // Error callback should have been triggered by RtAudio internals
        std::cerr << "RtAudio openStream failed with code: " << result << std::endl;
        streamIsOpen_ = false;
    } else {
        std::cout << "Stream opened successfully with buffer size: " << streamBufferFrames_ << std::endl;
        streamIsOpen_ = true;
    }

    return streamIsOpen_;
}

bool AudioManager::startStream() {
    if (!audio_ || !streamIsOpen_) {
        defaultErrorCallback(RTAUDIO_INVALID_USE, "Cannot start stream, stream not open.");
        return false;
    }
    if (streamIsRunning_) {
         defaultErrorCallback(RTAUDIO_WARNING, "Stream already running.");
         return true; // Already running is not failure
    }

    RtAudioErrorType result = RTAUDIO_NO_ERROR;
     try {
         result = audio_->startStream();
     } catch (const std::exception& e) {
          defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during startStream: " + std::string(e.what()));
          streamIsRunning_ = false; // Ensure state is correct
          return false;
     }

    if (result != RTAUDIO_NO_ERROR) {
         std::cerr << "RtAudio startStream failed with code: " << result << std::endl;
         streamIsRunning_ = false;
    } else {
        std::cout << "Stream started successfully." << std::endl;
        streamIsRunning_ = true;
    }
    return streamIsRunning_;
}

bool AudioManager::stopStream() {
     if (!audio_ || !streamIsOpen_) {
        // Don't warn if stream isn't open, just return success as it's already stopped state.
        return true;
    }
     if (!streamIsRunning_) {
         // Don't warn if already stopped.
         return true;
    }

    RtAudioErrorType result = RTAUDIO_NO_ERROR;
    std::cout << "Attempting to stop stream..." << std::endl;
     try {
         result = audio_->stopStream();
     } catch (const std::exception& e) {
          defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during stopStream: " + std::string(e.what()));
          // State might be uncertain here, but assume it stopped
          streamIsRunning_ = false;
          return false; // Indicate failure
     }

    if (result != RTAUDIO_NO_ERROR) {
        std::cerr << "RtAudio stopStream failed with code: " << result << std::endl;
        // State might be uncertain, maybe force running to false?
        streamIsRunning_ = false; // Assume it stopped despite error? Needs careful thought.
    } else {
        std::cout << "Stream stopped successfully." << std::endl;
        streamIsRunning_ = false;
    }
    // Return true if state is now not running, false otherwise
    return !streamIsRunning_;
}

void AudioManager::closeStream() {
     if (!audio_ || !streamIsOpen_) {
        return; // Nothing to close
    }

    if (streamIsRunning_) {
        std::cout << "Stream is running, stopping it before closing..." << std::endl;
        stopStream(); // Attempt graceful stop first
    }

    std::cout << "Closing stream..." << std::endl;
     try {
        audio_->closeStream();
     } catch (const std::exception& e) {
          // Log error, but continue cleanup
          defaultErrorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during closeStream: " + std::string(e.what()));
     }
    streamIsOpen_ = false;
    streamIsRunning_ = false; // Ensure state is fully reset
    std::cout << "Stream closed." << std::endl;

}

bool AudioManager::isStreamOpen() const {
    return streamIsOpen_ && audio_ && audio_->isStreamOpen(); // Check internal flag and RtAudio's state
}

bool AudioManager::isStreamRunning() const {
    return streamIsRunning_ && audio_ && audio_->isStreamRunning(); // Check internal flag and RtAudio's state
}

RtAudio::Api AudioManager::getCurrentApi() const {
    return audio_ ? actualApi_ : RtAudio::Api::UNSPECIFIED;
}

unsigned int AudioManager::getDefaultInputDeviceId() const {
    return audio_ ? audio_->getDefaultInputDevice() : 0;
}

unsigned int AudioManager::getDefaultOutputDeviceId() const {
    return audio_ ? audio_->getDefaultOutputDevice() : 0;
}