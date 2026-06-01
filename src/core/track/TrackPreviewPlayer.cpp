#include "track/TrackPreviewPlayer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <mutex>
#include <utility>
#include <vector>

#include <aubio/aubio.h>
#include <rtaudio/RtAudio.h>

#include "track/ChartNotePreviewSynth.h"
#include "track/TrackTiming.h"

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
        TrackPreviewMixSettings mixSettings;
        int timingTicksPerBeat = TempoMap::kDefaultTicksPerBeat;
        int timingChartAudioOffsetMs = 0;
        std::vector<TempoEvent> timingTempos = {{0, TempoMap::kDefaultBpm, "default"}};
        std::vector<TrackPreviewTimingMeasure> timingMeasures;
        std::vector<TrackPreviewTimingNote> timingNotes;
        struct ClickEvent
        {
            size_t frame = 0;
            float amplitude = 0.0f;
            bool note = false;
        };
        std::vector<ClickEvent> clickEvents;
        std::vector<ChartNotePreviewEvent> noteEvents;
        size_t clickCursor = 0;
        size_t noteEventCursor = 0;
        struct ActiveVoice
        {
            bool active = false;
            double frequencyHz = 0.0;
            double phase = 0.0;
            size_t ageFrames = 0;
            size_t durationFrames = 0;
            float amplitude = 0.0f;
        };
        std::array<ActiveVoice, 32> activeVoices{};

        float clickSampleAt(size_t frame) const
        {
            constexpr size_t kClickLength = 1200;
            float value = 0.0f;
            for (size_t index = clickCursor; index < clickEvents.size(); ++index)
            {
                const ClickEvent &event = clickEvents[index];
                if (event.frame > frame)
                {
                    break;
                }
                const size_t offset = frame - event.frame;
                if (offset < kClickLength)
                {
                    const float phase = static_cast<float>(offset) / static_cast<float>(kClickLength);
                    const float volume = event.note
                                             ? (mixSettings.notePreviewMuted ? 0.0f : mixSettings.notePreviewVolume)
                                             : (mixSettings.metronomeMuted ? 0.0f : mixSettings.metronomeVolume);
                    value += event.amplitude * volume * (1.0f - phase) *
                             (offset < 32 ? 1.0f : (offset % 18 < 9 ? 0.55f : -0.55f));
                }
            }
            return std::clamp(value, -1.0f, 1.0f);
        }

        void seekClickCursor()
        {
            constexpr size_t kClickLength = 1200;
            clickCursor = 0;
            while (clickCursor < clickEvents.size() &&
                   clickEvents[clickCursor].frame + kClickLength < frameCursor)
            {
                ++clickCursor;
            }
        }

        void seekNoteCursor()
        {
            noteEventCursor = 0;
            while (noteEventCursor < noteEvents.size() &&
                   noteEvents[noteEventCursor].frame < frameCursor)
            {
                ++noteEventCursor;
            }
            for (ActiveVoice &voice : activeVoices)
            {
                voice = {};
            }
        }

        size_t audioFrameForTick(const TempoMap &map, int tick) const
        {
            const double audioSeconds = map.tickToSeconds(tick) +
                                        static_cast<double>(timingChartAudioOffsetMs) / 1000.0;
            if (audioSeconds <= 0.0)
            {
                return 0;
            }
            return static_cast<size_t>(std::llround(audioSeconds * static_cast<double>(sampleRate)));
        }

        void startNoteEvent(const ChartNotePreviewEvent &event)
        {
            if (event.frequencyHz <= 0.0 || event.amplitude <= 0.0f || event.durationFrames == 0)
            {
                return;
            }
            auto voice = std::find_if(activeVoices.begin(), activeVoices.end(),
                                      [](const ActiveVoice &candidate)
                                      { return !candidate.active; });
            if (voice == activeVoices.end())
            {
                voice = std::min_element(activeVoices.begin(), activeVoices.end(),
                                         [](const ActiveVoice &left, const ActiveVoice &right)
                                         { return left.ageFrames > right.ageFrames; });
            }
            *voice = ActiveVoice{
                true,
                event.frequencyHz,
                0.0,
                0,
                event.durationFrames,
                event.amplitude,
            };
        }

        float noteSampleAt(size_t frame)
        {
            while (noteEventCursor < noteEvents.size() && noteEvents[noteEventCursor].frame <= frame)
            {
                startNoteEvent(noteEvents[noteEventCursor]);
                ++noteEventCursor;
            }

            float sample = 0.0f;
            for (ActiveVoice &voice : activeVoices)
            {
                if (!voice.active)
                {
                    continue;
                }
                if (voice.ageFrames >= voice.durationFrames)
                {
                    voice = {};
                    continue;
                }

                const double attackFrames = std::max(1.0, static_cast<double>(sampleRate) * 0.005);
                const double age = static_cast<double>(voice.ageFrames);
                const double duration = static_cast<double>(std::max<size_t>(1, voice.durationFrames));
                const double attack = std::min(1.0, age / attackFrames);
                const double decay = std::max(0.0, 1.0 - age / duration);
                const double envelope = attack * decay * decay;
                const double triangle = 4.0 * std::abs(voice.phase - 0.5) - 1.0;
                sample += static_cast<float>(triangle * envelope) * voice.amplitude;
                voice.phase += voice.frequencyHz / static_cast<double>(sampleRate);
                voice.phase -= std::floor(voice.phase);
                ++voice.ageFrames;
            }
            return std::clamp(sample, -1.0f, 1.0f);
        }

        void rebuildClickEvents()
        {
            clickEvents.clear();
            noteEvents.clear();
            clickCursor = 0;
            noteEventCursor = 0;
            if (sampleRate == 0)
            {
                return;
            }

            const TempoMap map(timingTicksPerBeat, timingTempos);
            const int audioEndTick = map.secondsToTick(durationSeconds());
            int endTick = audioEndTick;
            for (const TrackPreviewTimingNote &note : timingNotes)
            {
                endTick = std::max(endTick, note.tick + std::max(1, note.durationTicks));
            }

            const auto addClick = [&](int tick, float amplitude, bool note)
            {
                if (clickEvents.size() >= 200000 || tick < 0)
                {
                    return;
                }
                const double seconds = map.tickToSeconds(tick) +
                                       static_cast<double>(timingChartAudioOffsetMs) / 1000.0;
                if (seconds < 0.0 || seconds > durationSeconds() + 1.0)
                {
                    return;
                }
                clickEvents.push_back({
                    static_cast<size_t>(std::llround(seconds * static_cast<double>(sampleRate))),
                    amplitude,
                    note,
                });
            };

            if (mixSettings.metronomeEnabled)
            {
                if (timingMeasures.empty())
                {
                    for (int tick = 0; tick <= endTick; tick += std::max(1, timingTicksPerBeat))
                    {
                        addClick(tick, tick % (timingTicksPerBeat * 4) == 0 ? 0.9f : 0.55f, false);
                    }
                }
                else
                {
                    for (const TrackPreviewTimingMeasure &measure : timingMeasures)
                    {
                        const int numerator = std::max(1, measure.numerator);
                        const int denominator = std::max(1, measure.denominator);
                        for (int beat = 0; beat < numerator; ++beat)
                        {
                            const int tick = measure.startTick +
                                             beatOffsetTicksInMeasure(timingTicksPerBeat, beat, denominator);
                            addClick(tick, beat == 0 ? 0.9f : 0.55f, false);
                        }
                    }
                }
            }

            if (mixSettings.notePreviewEnabled)
            {
                for (const TrackPreviewTimingNote &note : timingNotes)
                {
                    if (note.frequencyHz <= 0.0 || note.midiNote < 0 ||
                        mixSettings.notePreviewMuted || mixSettings.notePreviewVolume <= 0.0f)
                    {
                        continue;
                    }
                    const double seconds = map.tickToSeconds(note.tick) +
                                           static_cast<double>(timingChartAudioOffsetMs) / 1000.0;
                    if (seconds < 0.0 || seconds > durationSeconds() + 1.0)
                    {
                        continue;
                    }
                    const double duration = std::clamp(
                        map.durationSecondsForTickRange(note.tick, note.tick + std::max(1, note.durationTicks)),
                        0.08,
                        0.18);
                    const auto durationFrames = static_cast<size_t>(
                        std::max<long long>(1, std::llround(duration * static_cast<double>(sampleRate))));
                    noteEvents.push_back({
                        audioFrameForTick(map, note.tick),
                        note.midiNote,
                        note.frequencyHz,
                        durationFrames,
                        std::clamp(mixSettings.notePreviewVolume, 0.0f, 1.0f) * 0.35f,
                    });
                }
            }

            std::sort(clickEvents.begin(), clickEvents.end(),
                      [](const ClickEvent &left, const ClickEvent &right)
                      { return left.frame < right.frame; });
            std::sort(noteEvents.begin(), noteEvents.end(),
                      [](const ChartNotePreviewEvent &left, const ChartNotePreviewEvent &right)
                      {
                          if (left.frame != right.frame)
                          {
                              return left.frame < right.frame;
                          }
                          return left.midiNote < right.midiNote;
                      });
            seekClickCursor();
            seekNoteCursor();
        }

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

                const size_t currentFrame = self->frameCursor;
                float sample = self->samples[self->frameCursor++];
                sample = self->mixSettings.songMuted ? 0.0f : sample * self->mixSettings.songVolume;
                sample = std::clamp(sample + self->clickSampleAt(currentFrame) + self->noteSampleAt(currentFrame),
                                    -1.0f,
                                    1.0f);
                constexpr size_t kClickLength = 1200;
                while (self->clickCursor < self->clickEvents.size() &&
                       self->clickEvents[self->clickCursor].frame + kClickLength < currentFrame)
                {
                    ++self->clickCursor;
                }
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
            rebuildClickEvents();

            return true;
        }

        double durationSeconds() const
        {
            return sampleRate == 0 ? 0.0 : static_cast<double>(totalFrames) / static_cast<double>(sampleRate);
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
            impl_->seekClickCursor();
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
            impl_->seekNoteCursor();
            impl_->seekClickCursor();
    }

    void TrackPreviewPlayer::update()
    {
        if (impl_->playbackComplete.exchange(false))
        {
            stop();
            impl_->status = "Preview finished";
        }
    }

    void TrackPreviewPlayer::setMixSettings(const TrackPreviewMixSettings &settings)
    {
        std::scoped_lock lock(impl_->mutex);
        impl_->mixSettings = settings;
        impl_->rebuildClickEvents();
    }

    TrackPreviewMixSettings TrackPreviewPlayer::mixSettings() const
    {
        std::scoped_lock lock(impl_->mutex);
        return impl_->mixSettings;
    }

    void TrackPreviewPlayer::setTimingPreview(int ticksPerBeat,
                                              std::vector<TempoEvent> tempoEvents,
                                              std::vector<TrackPreviewTimingMeasure> measures,
                                              std::vector<TrackPreviewTimingNote> notes,
                                              int chartAudioOffsetMs)
    {
        std::scoped_lock lock(impl_->mutex);
        impl_->timingTicksPerBeat = std::max(1, ticksPerBeat);
        impl_->timingChartAudioOffsetMs = chartAudioOffsetMs;
        impl_->timingTempos = std::move(tempoEvents);
        impl_->timingMeasures = std::move(measures);
        impl_->timingNotes = std::move(notes);
        impl_->rebuildClickEvents();
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
        return impl_->durationSeconds();
    }

    std::string TrackPreviewPlayer::status() const
    {
        return impl_->status;
    }
}
