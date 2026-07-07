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
    deleteSelectedNotes();
}

bool TrackEditorScene::hasSelectedNotes() const
{
    return !selectedNoteIds_.empty();
}

bool TrackEditorScene::activePartHasNotes() const
{
    const std::string activePart = currentPartName();
    return std::any_of(chart_.notes().begin(), chart_.notes().end(),
                       [&](const TrackTabNote &note)
                       {
                           return note.part == activePart;
                       });
}

void TrackEditorScene::copySelectedNotes()
{
    ensureNoteEditorIds();
    if (selectedNoteIds_.empty())
    {
        return;
    }

    const std::string activePart = currentPartName();
    std::vector<TrackTabNote> copied;
    for (const TrackTabNote &note : chart_.notes())
    {
        if (note.part == activePart && isNoteSelected(note.editorId))
        {
            copied.push_back(note);
        }
    }
    if (copied.empty())
    {
        return;
    }

    std::sort(copied.begin(), copied.end(),
              [](const TrackTabNote &left, const TrackTabNote &right)
              {
                  if (left.tick != right.tick)
                  {
                      return left.tick < right.tick;
                  }
                  if (left.stringIndex != right.stringIndex)
                  {
                      return left.stringIndex < right.stringIndex;
                  }
                  return left.editorId < right.editorId;
              });
    const int anchorTick = copied.front().tick;
    for (TrackTabNote &note : copied)
    {
        note.tick = std::max(0, note.tick - anchorTick);
        note.editorId = 0;
    }

    noteClipboard_.sourcePart = activePart;
    noteClipboard_.notes = std::move(copied);
    statusMessage_ = "Copied " + std::to_string(noteClipboard_.notes.size()) +
                     (noteClipboard_.notes.size() == 1 ? " note." : " notes.");
}

void TrackEditorScene::cutSelectedNotes()
{
    ensureNoteEditorIds();
    if (selectedNoteIds_.empty())
    {
        return;
    }
    const size_t cutCount = selectedNoteIds_.size();
    copySelectedNotes();
    deleteSelectedNotes();
    statusMessage_ = "Cut " + std::to_string(cutCount) +
                     (cutCount == 1 ? " note." : " notes.");
}

void TrackEditorScene::pasteCopiedNotes()
{
    ensureNoteEditorIds();
    if (noteClipboard_.notes.empty())
    {
        return;
    }

    const std::vector<TrackTabNote> beforeNotes = chart_.notes();
    const std::vector<openchordix::track::editor::EditorNoteId> beforeSelection = selectedNoteIds_;
    const std::string activePart = currentPartName();
    const int targetTick = quantizeTick(std::max(0, static_cast<int>(std::lround(timelineTickFromSeconds(displayedCursorSeconds())))));
    std::vector<openchordix::track::editor::EditorNoteId> pastedIds;
    pastedIds.reserve(noteClipboard_.notes.size());
    for (TrackTabNote note : noteClipboard_.notes)
    {
        note.editorId = nextEditorNoteId_++;
        note.part = activePart;
        note.tick = std::max(0, targetTick + note.tick);
        note.stringIndex = std::clamp(note.stringIndex, 0, currentPartStringCount() - 1);
        pastedIds.push_back(note.editorId);
        chart_.notes().push_back(std::move(note));
    }
    sortNotes();
    selectedNoteIds_ = pastedIds;
    syncSelectedNoteIndex();
    pushNoteEditCommand("Paste notes", beforeNotes, beforeSelection);
    statusMessage_ = "Pasted " + std::to_string(pastedIds.size()) +
                     (pastedIds.size() == 1 ? " note." : " notes.");
}

void TrackEditorScene::deleteSelectedNotes()
{
    ensureNoteEditorIds();
    if (selectedNoteIds_.empty())
    {
        return;
    }

    const std::vector<TrackTabNote> beforeNotes = chart_.notes();
    const std::vector<openchordix::track::editor::EditorNoteId> beforeSelection = selectedNoteIds_;
    const size_t beforeCount = chart_.notes().size();
    chart_.notes().erase(
        std::remove_if(chart_.notes().begin(), chart_.notes().end(),
                       [&](const TrackTabNote &note)
                       {
                           return openchordix::track::editor::containsNoteId(selectedNoteIds_, note.editorId);
                       }),
        chart_.notes().end());
    const size_t deletedCount = beforeCount - chart_.notes().size();
    clearNoteSelection();
    if (deletedCount > 0)
    {
        pushNoteEditCommand("Delete notes", beforeNotes, beforeSelection);
        statusMessage_ = "Deleted " + std::to_string(deletedCount) +
                         (deletedCount == 1 ? " note." : " notes.");
    }
}

void TrackEditorScene::selectAllCurrentPartNotes()
{
    ensureNoteEditorIds();
    const std::string activePart = currentPartName();
    std::vector<openchordix::track::editor::EditorPartNoteRef> noteRefs;
    noteRefs.reserve(chart_.notes().size());
    for (const TrackTabNote &note : chart_.notes())
    {
        noteRefs.push_back({note.editorId, note.part});
    }
    selectedNoteIds_ = openchordix::track::editor::noteIdsForPart(noteRefs, activePart);
    syncSelectedNoteIndex();
    statusMessage_ = "Selected " + std::to_string(selectedNoteIds_.size()) +
                     (selectedNoteIds_.size() == 1 ? " note." : " notes.");
}

void TrackEditorScene::ensureNoteEditorIds()
{
    for (TrackTabNote &note : chart_.notes())
    {
        if (note.editorId == 0)
        {
            note.editorId = nextEditorNoteId_++;
        }
        else
        {
            nextEditorNoteId_ = std::max(nextEditorNoteId_, note.editorId + 1);
        }
    }
    syncSelectedNoteIndex();
}

int TrackEditorScene::noteIndexByEditorId(openchordix::track::editor::EditorNoteId id) const
{
    if (id == 0)
    {
        return -1;
    }
    for (int index = 0; index < static_cast<int>(chart_.notes().size()); ++index)
    {
        if (chart_.notes()[static_cast<size_t>(index)].editorId == id)
        {
            return index;
        }
    }
    return -1;
}

bool TrackEditorScene::isNoteSelected(openchordix::track::editor::EditorNoteId id) const
{
    return openchordix::track::editor::containsNoteId(selectedNoteIds_, id);
}

void TrackEditorScene::clearNoteSelection()
{
    selectedNoteIds_.clear();
    selectedNoteIndex_ = -1;
}

void TrackEditorScene::setPrimarySelectedNote(openchordix::track::editor::EditorNoteId id)
{
    selectedNoteIds_.clear();
    openchordix::track::editor::addNoteIdIfMissing(selectedNoteIds_, id);
    syncSelectedNoteIndex();
}

void TrackEditorScene::syncSelectedNoteIndex()
{
    selectedNoteIds_.erase(
        std::remove_if(selectedNoteIds_.begin(), selectedNoteIds_.end(),
                       [&](openchordix::track::editor::EditorNoteId id)
                       {
                           return noteIndexByEditorId(id) < 0;
                       }),
        selectedNoteIds_.end());
    selectedNoteIndex_ = selectedNoteIds_.empty() ? -1 : noteIndexByEditorId(selectedNoteIds_.front());
}

int TrackEditorScene::commitDraggedSelectionMove(int deltaTicks)
{
    if (deltaTicks == 0 || draggedSelectionOriginalNotes_.empty())
    {
        return 0;
    }

    const std::vector<TrackTabNote> beforeNotes = chart_.notes();
    const std::vector<openchordix::track::editor::EditorNoteId> beforeSelection = selectedNoteIds_;
    const auto movedNotes = openchordix::track::editor::movedDraggedNotes(draggedSelectionOriginalNotes_, deltaTicks);
    int movedCount = 0;
    for (TrackTabNote &note : chart_.notes())
    {
        const auto moved = std::find_if(
            movedNotes.begin(),
            movedNotes.end(),
            [&](const openchordix::track::editor::EditorMovedNote &candidate)
            {
                return candidate.id == note.editorId;
            });
        if (moved == movedNotes.end())
        {
            continue;
        }

        note.tick = moved->tick;
        ++movedCount;
    }
    if (movedCount > 0)
    {
        sortNotes();
        syncSelectedNoteIndex();
        pushNoteEditCommand("Move notes", beforeNotes, beforeSelection);
    }
    return movedCount;
}

bool TrackEditorScene::canUndoNoteEdit() const
{
    return !undoStack_.empty();
}

bool TrackEditorScene::canRedoNoteEdit() const
{
    return !redoStack_.empty();
}

void TrackEditorScene::undoNoteEdit()
{
    if (undoStack_.empty())
    {
        return;
    }
    NoteEditCommand command = std::move(undoStack_.back());
    undoStack_.pop_back();
    restoreNoteEditState(command.beforeNotes, command.beforeSelection);
    statusMessage_ = "Undo: " + command.label;
    redoStack_.push_back(std::move(command));
}

void TrackEditorScene::redoNoteEdit()
{
    if (redoStack_.empty())
    {
        return;
    }
    NoteEditCommand command = std::move(redoStack_.back());
    redoStack_.pop_back();
    restoreNoteEditState(command.afterNotes, command.afterSelection);
    statusMessage_ = "Redo: " + command.label;
    undoStack_.push_back(std::move(command));
}

void TrackEditorScene::pushNoteEditCommand(std::string label,
                                           std::vector<TrackTabNote> beforeNotes,
                                           std::vector<openchordix::track::editor::EditorNoteId> beforeSelection)
{
    if (label.empty())
    {
        label = "Edit notes";
    }
    NoteEditCommand command;
    command.label = std::move(label);
    command.beforeNotes = std::move(beforeNotes);
    command.afterNotes = chart_.notes();
    command.beforeSelection = std::move(beforeSelection);
    command.afterSelection = selectedNoteIds_;
    undoStack_.push_back(std::move(command));
    redoStack_.clear();
}

void TrackEditorScene::restoreNoteEditState(const std::vector<TrackTabNote> &notes,
                                            const std::vector<openchordix::track::editor::EditorNoteId> &selection)
{
    chart_.notes() = notes;
    selectedNoteIds_ = selection;
    ensureNoteEditorIds();
    sortNotes();
    syncSelectedNoteIndex();
}

void TrackEditorScene::handleEditorShortcuts()
{
    const ImGuiIO &io = ImGui::GetIO();
    if (!openchordix::track::editor::shouldHandleEditorShortcut(
            io.WantTextInput,
            ImGui::IsAnyItemActive(),
            ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)))
    {
        return;
    }

    if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        redoNoteEdit();
        return;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
    {
        redoNoteEdit();
        return;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        undoNoteEdit();
        return;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
    {
        copySelectedNotes();
        return;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X, false))
    {
        cutSelectedNotes();
        return;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false))
    {
        pasteCopiedNotes();
        return;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
    {
        selectAllCurrentPartNotes();
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
    {
        deleteSelectedNotes();
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
    {
        clearNoteSelection();
        selectionDragActive_ = false;
        selectionDragExceededThreshold_ = false;
        draggingNote_ = false;
        draggingNoteId_ = 0;
        draggedSelectionOriginalNotes_.clear();
        currentDragDeltaTicks_ = 0;
    }
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
