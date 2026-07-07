#pragma once

#include <optional>
#include <string>
#include <vector>

namespace openchordix::track
{
    struct TempoEvent
    {
        int tick = 0;
        double bpm = 120.0;
        std::string source;
    };

    class TempoMap
    {
    public:
        static constexpr int kDefaultTicksPerBeat = 960;
        static constexpr double kDefaultBpm = 120.0;

        TempoMap();
        explicit TempoMap(int ticksPerBeat);
        TempoMap(int ticksPerBeat, std::vector<TempoEvent> events, double fallbackBpm = kDefaultBpm);

        int ticksPerBeat() const { return ticksPerBeat_; }
        void setTicksPerBeat(int value);

        const std::vector<TempoEvent> &events() const { return events_; }
        void setEvents(std::vector<TempoEvent> events, double fallbackBpm = kDefaultBpm);
        bool addEvent(TempoEvent event);

        bool validate(std::string *error = nullptr) const;

        double tickToSeconds(int tick) const;
        int secondsToTick(double seconds) const;
        double durationSecondsForTickRange(int startTick, int endTick) const;
        double bpmAtTick(int tick) const;
        std::optional<TempoEvent> nextEventAfterTick(int tick) const;

    private:
        struct InternalEvent
        {
            int tick = 0;
            double bpm = kDefaultBpm;
            long long microsecondsPerQuarter = 500000;
            std::string source;
        };

        static bool isValidBpm(double bpm);
        static long long microsecondsPerQuarter(double bpm);
        static std::vector<InternalEvent> normalizeEvents(const std::vector<TempoEvent> &events,
                                                          double fallbackBpm);
        void rebuildPublicEvents();

        int ticksPerBeat_ = kDefaultTicksPerBeat;
        std::vector<InternalEvent> internalEvents_;
        std::vector<TempoEvent> events_;
    };
}
