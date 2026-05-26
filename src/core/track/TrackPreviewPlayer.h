#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace openchordix::track
{
    class TrackPreviewPlayer
    {
    public:
        TrackPreviewPlayer();
        ~TrackPreviewPlayer();

        bool play(const std::filesystem::path &audioPath, double startSeconds = 0.0);
        void stop();
        void update();

        bool isPlaying() const;
        double currentTimeSeconds() const;
        double durationSeconds() const;
        std::string status() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
