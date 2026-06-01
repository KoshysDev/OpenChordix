#include "track/TempoMap.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace openchordix::track
{
    namespace
    {
        constexpr double kMinimumBpm = 1.0;
        constexpr double kMaximumBpm = 1000.0;
        constexpr long double kMicrosecondsPerSecond = 1000000.0L;
        constexpr long double kMicrosecondsPerMinute = 60000000.0L;

        int clampTick(long long tick)
        {
            return static_cast<int>(std::clamp<long long>(tick, 0, std::numeric_limits<int>::max()));
        }
    }

    TempoMap::TempoMap()
    {
        setEvents({TempoEvent{0, kDefaultBpm, "default"}});
    }

    TempoMap::TempoMap(int ticksPerBeat)
    {
        setTicksPerBeat(ticksPerBeat);
        setEvents({TempoEvent{0, kDefaultBpm, "default"}});
    }

    TempoMap::TempoMap(int ticksPerBeat, std::vector<TempoEvent> events, double fallbackBpm)
    {
        setTicksPerBeat(ticksPerBeat);
        setEvents(std::move(events), fallbackBpm);
    }

    void TempoMap::setTicksPerBeat(int value)
    {
        ticksPerBeat_ = std::max(1, value);
    }

    void TempoMap::setEvents(std::vector<TempoEvent> events, double fallbackBpm)
    {
        internalEvents_ = normalizeEvents(events, fallbackBpm);
        rebuildPublicEvents();
    }

    bool TempoMap::addEvent(TempoEvent event)
    {
        if (!isValidBpm(event.bpm))
        {
            return false;
        }
        events_.push_back(std::move(event));
        internalEvents_ = normalizeEvents(events_, kDefaultBpm);
        rebuildPublicEvents();
        return true;
    }

    bool TempoMap::validate(std::string *error) const
    {
        const auto fail = [&](const char *message)
        {
            if (error != nullptr)
            {
                *error = message;
            }
            return false;
        };

        if (ticksPerBeat_ <= 0)
        {
            return fail("Ticks per beat must be positive.");
        }
        if (internalEvents_.empty())
        {
            return fail("Tempo map must contain at least one tempo event.");
        }
        if (internalEvents_.front().tick != 0)
        {
            return fail("Tempo map must contain a tempo event at tick 0.");
        }
        for (std::size_t index = 0; index < internalEvents_.size(); ++index)
        {
            const InternalEvent &event = internalEvents_[index];
            if (event.tick < 0)
            {
                return fail("Tempo event ticks must be non-negative.");
            }
            if (!isValidBpm(event.bpm))
            {
                return fail("Tempo event BPM must be positive and reasonable.");
            }
            if (index > 0 && internalEvents_[index - 1].tick >= event.tick)
            {
                return fail("Tempo events must be strictly sorted by tick.");
            }
        }
        return true;
    }

    double TempoMap::tickToSeconds(int tick) const
    {
        const int targetTick = std::max(0, tick);
        if (internalEvents_.empty())
        {
            return 0.0;
        }

        long double elapsedMicroseconds = 0.0L;
        for (std::size_t index = 0; index < internalEvents_.size(); ++index)
        {
            const InternalEvent &event = internalEvents_[index];
            const int segmentStart = event.tick;
            const int segmentEnd = index + 1 < internalEvents_.size()
                                       ? internalEvents_[index + 1].tick
                                       : targetTick;
            if (targetTick <= segmentStart)
            {
                break;
            }
            const int endTick = std::min(targetTick, segmentEnd);
            const int deltaTicks = std::max(0, endTick - segmentStart);
            elapsedMicroseconds += static_cast<long double>(deltaTicks) *
                                   static_cast<long double>(event.microsecondsPerQuarter) /
                                   static_cast<long double>(ticksPerBeat_);
            if (targetTick <= segmentEnd)
            {
                break;
            }
        }
        return static_cast<double>(elapsedMicroseconds / kMicrosecondsPerSecond);
    }

    int TempoMap::secondsToTick(double seconds) const
    {
        if (seconds <= 0.0 || internalEvents_.empty())
        {
            return 0;
        }

        long double remainingMicroseconds =
            static_cast<long double>(seconds) * kMicrosecondsPerSecond;
        for (std::size_t index = 0; index < internalEvents_.size(); ++index)
        {
            const InternalEvent &event = internalEvents_[index];
            const bool hasNext = index + 1 < internalEvents_.size();
            const int nextTick = hasNext ? internalEvents_[index + 1].tick : std::numeric_limits<int>::max();
            const int segmentTicks = std::max(0, nextTick - event.tick);
            const long double segmentMicroseconds =
                static_cast<long double>(segmentTicks) *
                static_cast<long double>(event.microsecondsPerQuarter) /
                static_cast<long double>(ticksPerBeat_);

            if (!hasNext || remainingMicroseconds <= segmentMicroseconds)
            {
                const long double ticks =
                    remainingMicroseconds * static_cast<long double>(ticksPerBeat_) /
                    static_cast<long double>(event.microsecondsPerQuarter);
                return clampTick(static_cast<long long>(event.tick) +
                                 static_cast<long long>(std::llround(ticks)));
            }

            remainingMicroseconds -= segmentMicroseconds;
        }
        return 0;
    }

    double TempoMap::durationSecondsForTickRange(int startTick, int endTick) const
    {
        const int start = std::max(0, startTick);
        const int end = std::max(start, endTick);
        return tickToSeconds(end) - tickToSeconds(start);
    }

    double TempoMap::bpmAtTick(int tick) const
    {
        if (internalEvents_.empty())
        {
            return kDefaultBpm;
        }
        const int targetTick = std::max(0, tick);
        const auto found = std::upper_bound(
            internalEvents_.begin(), internalEvents_.end(), targetTick,
            [](int value, const InternalEvent &event)
            {
                return value < event.tick;
            });
        if (found == internalEvents_.begin())
        {
            return found->bpm;
        }
        return std::prev(found)->bpm;
    }

    std::optional<TempoEvent> TempoMap::nextEventAfterTick(int tick) const
    {
        const int targetTick = std::max(0, tick);
        const auto found = std::upper_bound(
            events_.begin(), events_.end(), targetTick,
            [](int value, const TempoEvent &event)
            {
                return value < event.tick;
            });
        if (found == events_.end())
        {
            return std::nullopt;
        }
        return *found;
    }

    bool TempoMap::isValidBpm(double bpm)
    {
        return std::isfinite(bpm) && bpm >= kMinimumBpm && bpm <= kMaximumBpm;
    }

    long long TempoMap::microsecondsPerQuarter(double bpm)
    {
        return static_cast<long long>(std::llround(kMicrosecondsPerMinute / bpm));
    }

    std::vector<TempoMap::InternalEvent> TempoMap::normalizeEvents(const std::vector<TempoEvent> &events,
                                                                   double fallbackBpm)
    {
        struct IndexedEvent
        {
            std::size_t index = 0;
            TempoEvent event;
        };

        std::vector<IndexedEvent> valid;
        valid.reserve(events.size() + 1);
        for (std::size_t index = 0; index < events.size(); ++index)
        {
            TempoEvent event = events[index];
            if (!isValidBpm(event.bpm))
            {
                continue;
            }
            event.tick = std::max(0, event.tick);
            valid.push_back({index, std::move(event)});
        }

        const double safeFallback = isValidBpm(fallbackBpm) ? fallbackBpm : kDefaultBpm;
        if (valid.empty())
        {
            valid.push_back({0, TempoEvent{0, safeFallback, "fallback"}});
        }

        const bool hasZero = std::any_of(valid.begin(), valid.end(),
                                         [](const IndexedEvent &event)
                                         { return event.event.tick == 0; });
        if (!hasZero)
        {
            valid.push_back({events.size(), TempoEvent{0, safeFallback, "fallback"}});
        }

        std::stable_sort(valid.begin(), valid.end(),
                         [](const IndexedEvent &left, const IndexedEvent &right)
                         {
                             if (left.event.tick != right.event.tick)
                             {
                                 return left.event.tick < right.event.tick;
                             }
                             return left.index < right.index;
                         });

        std::vector<InternalEvent> normalized;
        for (const IndexedEvent &entry : valid)
        {
            InternalEvent event{
                entry.event.tick,
                entry.event.bpm,
                microsecondsPerQuarter(entry.event.bpm),
                entry.event.source,
            };
            if (!normalized.empty() && normalized.back().tick == event.tick)
            {
                normalized.back() = std::move(event);
            }
            else
            {
                normalized.push_back(std::move(event));
            }
        }
        return normalized;
    }

    void TempoMap::rebuildPublicEvents()
    {
        events_.clear();
        events_.reserve(internalEvents_.size());
        for (const InternalEvent &event : internalEvents_)
        {
            events_.push_back({event.tick, event.bpm, event.source});
        }
    }
}
