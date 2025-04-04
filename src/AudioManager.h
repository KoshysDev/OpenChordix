#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <string>
#include <vector>
#include <memory>     
#include <functional> 
#include <rtaudio/RtAudio.h>

// Define a structure to pass essential info to the static callback
struct AudioCallbackData {
    unsigned int inputChannels = 0;
    unsigned int outputChannels = 0;
};


class AudioManager {
public:
    explicit AudioManager(RtAudio::Api api = RtAudio::Api::UNSPECIFIED);
    ~AudioManager() = default; // Default destructor is fine
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = default; // Allow moving
    AudioManager& operator=(AudioManager&&) = default; // Allow moving

    // --- Device Info / Listing ---
    bool listDevices() const;
    static std::vector<RtAudio::Api> getAvailableApis();
    // Add a helper to get info for a specific device
    RtAudio::DeviceInfo getDeviceInfo(unsigned int deviceId) const;


    // --- Stream Management ---
    bool openMonitoringStream(unsigned int inputDeviceId,
                              unsigned int sampleRate = 44100,
                              unsigned int bufferFrames = 256);
    bool startStream(); // Returns true on success
    bool stopStream();  // Returns true on success
    void closeStream();
    bool isStreamOpen() const;
    bool isStreamRunning() const;


    // --- Getters ---
    RtAudio::Api getCurrentApi() const;
    unsigned int getDefaultInputDeviceId() const;
    unsigned int getDefaultOutputDeviceId() const;


private:
    // --- Private Members ---
    std::unique_ptr<RtAudio> audio_;
    RtAudio::Api selectedApi_;
    RtAudio::Api actualApi_;
    bool streamIsOpen_ = false;
    bool streamIsRunning_ = false;
    unsigned int streamInputChannels_ = 0;
    unsigned int streamOutputChannels_ = 0;
    unsigned int streamSampleRate_ = 0; // Store actual sample rate
    unsigned int streamBufferFrames_ = 0; // Store actual buffer frames

    AudioCallbackData callbackData_; // Store data needed by the callback

    // --- Static Callback Functions ---
    static void defaultErrorCallback(RtAudioErrorType type, const std::string &errorText);
    // The actual audio processing callback for monitoring
    static int monitoringCallback( void *outputBuffer, void *inputBuffer, unsigned int nFrames,
                                   double streamTime, RtAudioStreamStatus status, void *userData );
};

#endif