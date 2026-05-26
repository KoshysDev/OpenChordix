#pragma once

int TrackEditorScene::snapTickSize() const
{
    return openchordix::track::editor::snapTickSize(chart_.ticksPerBeat(), snapIndex_);
}

int TrackEditorScene::quantizeTick(int tick) const
{
    return openchordix::track::editor::quantizeTick(tick, chart_.ticksPerBeat(), snapIndex_);
}

void TrackEditorScene::removeSelectedNote()
{
    if (selectedNoteIndex_ < 0 || selectedNoteIndex_ >= static_cast<int>(chart_.notes().size()))
    {
        return;
    }
    chart_.notes().erase(chart_.notes().begin() + selectedNoteIndex_);
    selectedNoteIndex_ = -1;
}

void TrackEditorScene::refreshDurationFromAudio()
{
    const std::filesystem::path audioPath = currentResolvedAudioPath();
    if (auto seconds = openchordix::track::readAudioDurationSeconds(audioPath))
    {
        durationSeconds_ = *seconds;
    }
}

double TrackEditorScene::displayedCursorSeconds() const
{
    return previewPlayer_->isPlaying()
               ? clampTransportCursor(previewPlayer_->currentTimeSeconds())
               : transportCursorSeconds_;
}

int TrackEditorScene::timelineTotalBeats() const
{
    return openchordix::track::editor::timelineTotalBeats(songDurationSeconds(), draftBpm_, chart_.beatsPerMeasure());
}

int TrackEditorScene::timelineTotalTicks() const
{
    return std::max(
        openchordix::track::editor::timelineTotalTicks(songDurationSeconds(), draftBpm_, chart_.beatsPerMeasure(), chart_.ticksPerBeat()),
        chart_.timelineEndTick());
}

double TrackEditorScene::timelineTickFromSeconds(double seconds) const
{
    return openchordix::track::editor::timelineTickFromSeconds(seconds, draftBpm_, chart_.ticksPerBeat(), songDurationSeconds());
}

double TrackEditorScene::timelineSecondsFromTick(double tick) const
{
    return openchordix::track::editor::timelineSecondsFromTick(tick, draftBpm_, chart_.ticksPerBeat());
}

void TrackEditorScene::pausePreviewPlayback()
{
    if (!previewPlayer_->isPlaying())
    {
        return;
    }

    transportCursorSeconds_ = clampTransportCursor(previewPlayer_->currentTimeSeconds());
    previewPlayer_->stop();
}

void TrackEditorScene::togglePreviewPlayback()
{
    if (previewPlayer_->isPlaying())
    {
        pausePreviewPlayback();
        return;
    }

    startPreviewFromCursor();
}

void TrackEditorScene::stopPreviewToStart()
{
    previewPlayer_->stop();
    transportCursorSeconds_ = clampTransportCursor(chart_.previewStartSeconds());
    requestTimelineSync();
}

void TrackEditorScene::requestTimelineSync()
{
    timelineSyncPending_ = true;
}

void TrackEditorScene::syncTimelineScrollToSeconds(double seconds, float viewportWidth)
{
    const float effectiveViewportWidth = viewportWidth > 0.0f ? viewportWidth : timelineViewportWidth_;
    if (effectiveViewportWidth <= 0.0f)
    {
        timelineSyncPending_ = true;
        return;
    }

    const float pixelsPerBeat = 94.0f * zoom_;
    const float pixelsPerTick = pixelsPerBeat / static_cast<float>(chart_.ticksPerBeat());
    const float canvasWidth = track_editor::kLaneLabelWidth + timelineTotalTicks() * pixelsPerTick + 180.0f;
    const float maxScroll = std::max(0.0f, canvasWidth - effectiveViewportWidth);
    const float targetX = track_editor::kLaneLabelWidth + static_cast<float>(timelineTickFromSeconds(seconds)) * pixelsPerTick;
    timelineScrollX_ = std::clamp(targetX - effectiveViewportWidth * 0.5f, 0.0f, maxScroll);
    timelineSyncPending_ = false;
}

int TrackEditorScene::songDurationSeconds() const
{
    if (durationSeconds_ > 0)
    {
        return durationSeconds_;
    }
    if (previewPlayer_->durationSeconds() > 0.0)
    {
        return static_cast<int>(std::round(previewPlayer_->durationSeconds()));
    }
    return 90;
}

double TrackEditorScene::clampTransportCursor(double seconds) const
{
    return openchordix::track::editor::clampTransportCursor(seconds, songDurationSeconds());
}

bool TrackEditorScene::startPreviewFromCursor()
{
    const std::filesystem::path audioPath = currentResolvedAudioPath();
    if (audioPath.empty())
    {
        statusMessage_ = "No preview audio configured.";
        return false;
    }

    transportCursorSeconds_ = clampTransportCursor(transportCursorSeconds_);
    if (!previewPlayer_->play(audioPath, transportCursorSeconds_))
    {
        statusMessage_ = previewPlayer_->status();
        return false;
    }

    statusMessage_.clear();
    return true;
}
