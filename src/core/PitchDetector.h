#ifndef PITCHDETECTOR_H
#define PITCHDETECTOR_H

#include <aubio/aubio.h>
#include <atomic>
#include <string>

class PitchDetector
{
public:
    PitchDetector(uint_t bufferSize, uint_t hopSize, uint_t sampleRate, const std::string &method = "yin");

    ~PitchDetector();

    // Prevent copying and assignment
    PitchDetector(const PitchDetector &) = delete;
    PitchDetector &operator=(const PitchDetector &) = delete;

    void process(const float *inputBuffer, uint_t numFrames, uint_t inputChannelCount);

    float getPitchHz() const;

private:
    aubio_pitch_t *pitch_object_ = nullptr;
    fvec_t *aubio_input_buffer_ = nullptr;
    fvec_t *aubio_pitch_output_ = nullptr;

    std::atomic<float> latest_pitch_hz_{0.0f};
    float smoothed_pitch_hz_ = 0.0f;
    bool has_smoothed_ = false;

    // Configuration stored
    uint_t config_buffer_size_;
    uint_t config_hop_size_;
    uint_t config_sample_rate_;
    float smoothing_alpha_ = 0.22f;
};

#endif
