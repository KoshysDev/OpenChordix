#include "track/TrackPreviewPlayer.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <utility>
#include <vector>

#include <aubio/aubio.h>
#include <rtaudio/RtAudio.h>

namespace openchordix::track
{
    struct TrackPreviewPlayer::Impl
    {
        std::unique_ptr<RtAudio> audio;
        std::vector<float> samples;
        std::mutex mutex;
        std::atomic<bool> playbackComplete = false;
        std::string status = "Idle";
        unsigned int sampleRate = 0;
        unsigned int outputChannels = 2;
        size_t frameCursor = 0;
        size_t startFrame = 0;
        size_t totalFrames = 0;

        static int streamCallback(void *outputBuffer,
                                  void * /*inputBuffer*/,
                                  unsigned int nBufferFrames,
                                  double /*streamTime*/,
                                  RtAudioStreamStatus /*status*/,
                                  void *userData)
        {
            auto *self = static_cast<Impl *>(userData);
            auto *out = static_cast<float *>(outputBuffer);
            std::fill(out, out + nBufferFrames * self->outputChannels, 0.0f);

            std::scoped_lock lock(self->mutex);
            if (self->samples.empty())
            {
                self->playbackComplete.store(true);
                return 0;
            }

            for (unsigned int frame = 0; frame < nBufferFrames; ++frame)
            {
                if (self->frameCursor >= self->samples.size())
                {
                    self->playbackComplete.store(true);
                    break;
                }

                const float sample = self->samples[self->frameCursor++];
                for (unsigned int channel = 0; channel < self->outputChannels; ++channel)
                {
                    out[frame * self->outputChannels + channel] = sample;
                }
            }
            return 0;
        }

        bool loadSamples(const std::filesystem::path &audioPath)
        {
            samples.clear();
            frameCursor = 0;
            startFrame = 0;
            totalFrames = 0;

            const std::string path = audioPath.string();
            aubio_source_t *source = new_aubio_source(path.c_str(), 0, 512);
            if (source == nullptr)
            {
                status = "Failed to decode preview audio.";
                return false;
            }

            sampleRate = aubio_source_get_samplerate(source);
            fvec_t *buffer = new_fvec(512);
            if (buffer == nullptr || sampleRate == 0)
            {
                if (buffer != nullptr)
                {
                    del_fvec(buffer);
                }
                del_aubio_source(source);
                status = "Preview audio has invalid format.";
                return false;
            }

            uint_t read = 0;
            do
            {
                aubio_source_do(source, buffer, &read);
                for (uint_t i = 0; i < read; ++i)
                {
                    samples.push_back(buffer->data[i]);
                }
            } while (read == 512);

            del_fvec(buffer);
            del_aubio_source(source);

            if (samples.empty())
            {
                status = "Preview audio has no samples.";
                return false;
            }

            totalFrames = samples.size();

            return true;
        }
    };

    TrackPreviewPlayer::TrackPreviewPlayer()
        : impl_(std::make_unique<Impl>())
    {
    }

    TrackPreviewPlayer::~TrackPreviewPlayer()
    {
        stop();
    }

    bool TrackPreviewPlayer::play(const std::filesystem::path &audioPath, double startSeconds)
    {
        stop();
        if (audioPath.empty())
        {
            impl_->status = "No preview audio configured.";
            return false;
        }
        if (!std::filesystem::exists(audioPath))
        {
            impl_->status = "Preview audio file was not found.";
            return false;
        }
        if (!impl_->loadSamples(audioPath))
        {
            return false;
        }

        startSeconds = std::max(0.0, startSeconds);
        if (impl_->sampleRate > 0)
        {
            const size_t requestedFrame = static_cast<size_t>(startSeconds * static_cast<double>(impl_->sampleRate));
            impl_->startFrame = std::min(requestedFrame, impl_->totalFrames);
            impl_->frameCursor = impl_->startFrame;
        }

        impl_->audio = std::make_unique<RtAudio>();
        impl_->audio->showWarnings(false);

        unsigned int outputDeviceId = impl_->audio->getDefaultOutputDevice();
        if (outputDeviceId == 0 && impl_->audio->getDeviceCount() == 0)
        {
            impl_->status = "No output device available for preview.";
            impl_->audio.reset();
            return false;
        }

        RtAudio::StreamParameters outputParams{};
        outputParams.deviceId = outputDeviceId;
        outputParams.firstChannel = 0;
        outputParams.nChannels = impl_->outputChannels;

        unsigned int bufferFrames = 512;
        const RtAudioErrorType openResult = impl_->audio->openStream(
            &outputParams,
            nullptr,
            RTAUDIO_FLOAT32,
            impl_->sampleRate,
            &bufferFrames,
            &Impl::streamCallback,
            impl_.get());
        if (openResult != RTAUDIO_NO_ERROR)
        {
            impl_->status = impl_->audio->getErrorText();
            impl_->audio.reset();
            return false;
        }

        const RtAudioErrorType startResult = impl_->audio->startStream();
        if (startResult != RTAUDIO_NO_ERROR)
        {
            impl_->status = impl_->audio->getErrorText();
            if (impl_->audio->isStreamOpen())
            {
                impl_->audio->closeStream();
            }
            impl_->audio.reset();
            return false;
        }

        impl_->playbackComplete.store(false);
        impl_->status = "Preview playing";

        return true;
    }

    void TrackPreviewPlayer::stop()
    {
        if (!impl_->audio)
        {
            return;
        }
        if (impl_->audio->isStreamRunning())
        {
            if (impl_->audio->stopStream() != RTAUDIO_NO_ERROR)
            {
                impl_->status = impl_->audio->getErrorText();
            }
        }
        if (impl_->audio->isStreamOpen())
        {
            impl_->audio->closeStream();
        }
        impl_->audio.reset();
        impl_->frameCursor = 0;
    }

    void TrackPreviewPlayer::update()
    {
        if (impl_->playbackComplete.exchange(false))
        {
            stop();
            impl_->status = "Preview finished";
        }
    }

    bool TrackPreviewPlayer::isPlaying() const
    {
        return impl_->audio && impl_->audio->isStreamRunning();
    }

    double TrackPreviewPlayer::currentTimeSeconds() const
    {
        std::scoped_lock lock(impl_->mutex);
        if (impl_->sampleRate == 0)
        {
            return 0.0;
        }
        return static_cast<double>(impl_->frameCursor) / static_cast<double>(impl_->sampleRate);
    }

    double TrackPreviewPlayer::durationSeconds() const
    {
        std::scoped_lock lock(impl_->mutex);
        if (impl_->sampleRate == 0)
        {
            return 0.0;
        }
        return static_cast<double>(impl_->totalFrames) / static_cast<double>(impl_->sampleRate);
    }

    std::string TrackPreviewPlayer::status() const
    {
        return impl_->status;
    }
}
