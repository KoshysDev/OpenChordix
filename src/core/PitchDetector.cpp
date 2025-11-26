#include "PitchDetector.h"
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

PitchDetector::PitchDetector(uint_t bufferSize, uint_t hopSize, uint_t sampleRate, const std::string &method)
    : config_buffer_size_(bufferSize),
      config_hop_size_(hopSize),
      config_sample_rate_(sampleRate)
{
    // --- Input Validation ---
    if (bufferSize == 0 || hopSize == 0 || sampleRate == 0)
    {
        throw std::runtime_error("PitchDetector: Invalid zero parameter (bufferSize, hopSize, or sampleRate).");
    }

    std::cout << "Initializing Aubio pitch detection (" << method << "):"
              << " BufSize=" << bufferSize
              << " HopSize=" << hopSize
              << " SampleRate=" << sampleRate << std::endl;

    // --- Create Aubio Objects ---
    pitch_object_ = new_aubio_pitch("schmitt", bufferSize, hopSize, sampleRate);

    if (!pitch_object_)
    {
        throw std::runtime_error("PitchDetector: Failed to create Aubio pitch object.");
    }

    aubio_input_buffer_ = new_fvec(hopSize);
    if (!aubio_input_buffer_)
    {
        del_aubio_pitch(pitch_object_); // Clean up
        throw std::runtime_error("PitchDetector: Failed to create Aubio input buffer (size " + std::to_string(hopSize) + ").");
    }
    fvec_zeros(aubio_input_buffer_);

    aubio_pitch_output_ = new_fvec(1);
    if (!aubio_pitch_output_)
    {
        del_fvec(aubio_input_buffer_);
        del_aubio_pitch(pitch_object_);
        throw std::runtime_error("PitchDetector: Failed to create Aubio pitch output buffer.");
    }
    fvec_zeros(aubio_pitch_output_);

    std::cout << "Aubio PitchDetector initialized successfully." << std::endl;
}

PitchDetector::~PitchDetector()
{
    std::cout << "Destroying PitchDetector and Aubio objects..." << std::endl;
    if (pitch_object_)
    {
        del_aubio_pitch(pitch_object_);
    }
    if (aubio_input_buffer_)
    {
        del_fvec(aubio_input_buffer_);
    }
    if (aubio_pitch_output_)
    {
        del_fvec(aubio_pitch_output_);
    }
}

// Process buffer from RtAudio
void PitchDetector::process(const float *inputBuffer, uint_t numFrames, uint_t inputChannelCount)
{
    if (!pitch_object_ || !aubio_input_buffer_ || !aubio_pitch_output_ || inputBuffer == nullptr)
    {
        return;
    }

    if (numFrames != config_hop_size_)
    {
        std::cerr << "Warning: PitchDetector::process received numFrames (" << numFrames
                  << ") != configured hop_size (" << config_hop_size_ << "). Frame skipped." << std::endl;
        return;
    }

    if (inputChannelCount < 1)
        return;

    // Copy data from the 1 channel to Aubio input fvec
    for (uint_t i = 0; i < numFrames; ++i)
    {
        aubio_input_buffer_->data[i] = inputBuffer[i * inputChannelCount];
    }

    aubio_pitch_do(pitch_object_, aubio_input_buffer_, aubio_pitch_output_);

    // Get the pitch result(Hz)
    float detected_pitch = aubio_pitch_output_->data[0];

    // Exponential smoothing to reduce jitter while staying responsive
    if (detected_pitch > 0.0f)
    {
        if (has_smoothed_)
        {
            float semitone_jump = 12.0f * std::log2(detected_pitch / std::max(1e-6f, smoothed_pitch_hz_));
            // If the jump is extreme (e.g., octave flip), snap to new value to avoid lag
            if (std::fabs(semitone_jump) > 12.0f)
            {
                smoothed_pitch_hz_ = detected_pitch;
            }
            else
            {
                smoothed_pitch_hz_ += smoothing_alpha_ * (detected_pitch - smoothed_pitch_hz_);
            }
        }
        else
        {
            smoothed_pitch_hz_ = detected_pitch;
            has_smoothed_ = true;
        }
    }
    else
    {
        smoothed_pitch_hz_ = 0.0f;
        has_smoothed_ = false;
    }

    latest_pitch_hz_.store(smoothed_pitch_hz_);
}

float PitchDetector::getPitchHz() const
{
    return latest_pitch_hz_.load();
}
