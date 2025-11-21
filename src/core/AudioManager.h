#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <rtaudio/RtAudio.h>

#include "PitchDetector.h"

// Forward declare PitchDetector
class PitchDetector;

struct AudioCallbackData {
    unsigned int inputChannels = 0;
    unsigned int outputChannels = 0;
    PitchDetector* pitchDetector = nullptr; // Detector pointer 
};

class AudioManager {
public:
    explicit AudioManager(RtAudio::Api api = RtAudio::Api::UNSPECIFIED);
    
    ~AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = default;
    AudioManager& operator=(AudioManager&&) = default;

    // --- Device Info / Listing ---
    bool listDevices() const;
    static std::vector<RtAudio::Api> getAvailableApis();
    std::vector<unsigned int> getDeviceIds() const;
    RtAudio::DeviceInfo getDeviceInfo(unsigned int deviceId) const;

    // --- Stream Management ---
    bool openMonitoringStream(unsigned int inputDeviceId,
                              unsigned int sampleRate = 44100,
                              unsigned int bufferFrames = 256);
    bool startStream();
    bool stopStream();
    void closeStream();
    bool isStreamOpen() const;
    bool isStreamRunning() const;

    // --- Getters ---
    RtAudio::Api getCurrentApi() const;
    unsigned int getDefaultInputDeviceId() const;
    unsigned int getDefaultOutputDeviceId() const;
    // Get pitch directly from the detector
    float getLatestPitchHz() const;

private:
    // --- Private Members ---
    std::unique_ptr<RtAudio> audio_;
    std::unique_ptr<PitchDetector> pitch_detector_;
    RtAudio::Api selectedApi_;
    RtAudio::Api actualApi_;
    bool streamIsOpen_ = false;
    bool streamIsRunning_ = false;
    // Store channel counts used for RtAudio stream
    unsigned int streamInputChannels_ = 0;
    unsigned int streamOutputChannels_ = 0;
    unsigned int streamSampleRate_ = 0;
    unsigned int streamBufferFrames_ = 0;

    AudioCallbackData callbackData_;

    // --- Static Callbacks ---
    static void defaultErrorCallback(RtAudioErrorType type, const std::string &errorText);
    static int monitoringCallback( void *outputBuffer, void *inputBuffer, unsigned int nFrames,
                                   double streamTime, RtAudioStreamStatus status, void *userData );
};

#endif
