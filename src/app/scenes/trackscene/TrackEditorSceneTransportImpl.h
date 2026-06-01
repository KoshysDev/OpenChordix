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
    const int totalTicks = timelineTotalTicks();
    return std::max(chart_.beatsPerMeasure() * 4,
                    static_cast<int>(std::ceil(static_cast<double>(totalTicks) /
                                               static_cast<double>(chart_.ticksPerBeat()))));
}

int TrackEditorScene::timelineTotalTicks() const
{
    const openchordix::track::TempoMap map = currentTempoMap();
    return std::max(
        map.secondsToTick(static_cast<double>(songDurationSeconds())),
        chart_.timelineEndTick());
}

openchordix::track::TempoMap TrackEditorScene::currentTempoMap() const
{
    return chart_.tempoMap(static_cast<double>(std::max(1, draftBpm_)));
}

double TrackEditorScene::timelineTickFromSeconds(double seconds) const
{
    const double audioSeconds = clampTransportCursor(seconds);
    const double chartSeconds =
        openchordix::track::editor::chartSecondsFromAudioSeconds(audioSeconds, chart_.chartAudioOffsetMs());
    return static_cast<double>(currentTempoMap().secondsToTick(chartSeconds));
}

double TrackEditorScene::timelineSecondsFromTick(double tick) const
{
    const double chartSeconds = currentTempoMap().tickToSeconds(static_cast<int>(std::lround(std::max(0.0, tick))));
    return openchordix::track::editor::audioSecondsFromChartSeconds(chartSeconds, chart_.chartAudioOffsetMs());
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
    refreshTimingPreview();
    if (!previewPlayer_->play(audioPath, transportCursorSeconds_))
    {
        statusMessage_ = previewPlayer_->status();
        return false;
    }

    statusMessage_.clear();
    return true;
}

void TrackEditorScene::refreshTimingPreview()
{
    openchordix::track::TrackPreviewMixSettings settings;
    settings.songVolume = std::clamp(songVolume_, 0.0f, 1.0f);
    settings.songMuted = songMuted_;
    settings.metronomeVolume = std::clamp(metronomeVolume_, 0.0f, 1.0f);
    settings.metronomeMuted = metronomeMuted_;
    settings.metronomeEnabled = metronomeEnabled_;
    settings.notePreviewVolume = std::clamp(notePreviewVolume_, 0.0f, 1.0f);
    settings.notePreviewMuted = notePreviewMuted_;
    settings.notePreviewEnabled = noteClicksEnabled_;
    previewPlayer_->setMixSettings(settings);

    std::vector<openchordix::track::TrackPreviewTimingMeasure> measures;
    measures.reserve(chart_.measures().size());
    for (const TrackChartMeasure &measure : chart_.measures())
    {
        measures.push_back({
            measure.startTick,
            measure.durationTicks,
            measure.numerator,
            measure.denominator,
        });
    }

    std::vector<openchordix::track::TrackPreviewTimingNote> notes;
    notes.reserve(chart_.notes().size());
    const std::vector<TrackPart> parts = collectParts();
    for (const TrackTabNote &note : chart_.notes())
    {
        const auto part = std::find_if(parts.begin(), parts.end(),
                                       [&](const TrackPart &candidate)
                                       { return candidate.name == note.part; });
        const std::vector<std::string> tuning = part != parts.end()
                                                    ? openchordix::track::normalizeTrackTuning(part->tuning, part->name, part->stringCount)
                                                    : openchordix::track::defaultTuningForPart(note.part, openchordix::track::kDefaultTrackStrings);
        const auto midi = openchordix::track::chartNoteMidiForStringFret(tuning, note.stringIndex, note.fret);
        notes.push_back({
            note.tick,
            note.duration,
            note.stringIndex,
            note.fret,
            midi.value_or(-1),
            midi.has_value() ? openchordix::track::chartNoteFrequencyForMidi(*midi) : 0.0,
        });
    }

    previewPlayer_->setTimingPreview(chart_.ticksPerBeat(),
                                     currentTempoMap().events(),
                                     std::move(measures),
                                     std::move(notes),
                                     chart_.chartAudioOffsetMs());
}
