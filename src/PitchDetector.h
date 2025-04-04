#ifndef PITCHDETECTOR_H
#define PITCHDETECTOR_H

#include <aubio/aubio.h>
#include <atomic>
#include <string>

class PitchDetector {
    public:
        // Constructor: Initializes Aubio objects
        PitchDetector(uint_t bufferSize, uint_t hopSize, uint_t sampleRate, const std::string& method = "yin");
    
        // Destructor: Cleans up Aubio objects
        ~PitchDetector();
    
        // Prevent copying and assignment
        PitchDetector(const PitchDetector&) = delete;
        PitchDetector& operator=(const PitchDetector&) = delete;
    
        // Process an audio buffer
        void process(const float* inputBuffer, uint_t numFrames, uint_t inputChannelCount);
    
        // Get the latest detected pitch (thread-safe)
        float getPitchHz() const;
    
    private:
        // Aubio objects
        aubio_pitch_t *pitch_object_ = nullptr;
        fvec_t *aubio_input_buffer_ = nullptr;
        fvec_t *aubio_pitch_output_ = nullptr;
    
        // Store latest pitch atomically for thread safety
        std::atomic<float> latest_pitch_hz_{0.0f};
    
        // Configuration stored
        uint_t config_buffer_size_;
        uint_t config_hop_size_;
        uint_t config_sample_rate_;
    };

#endif