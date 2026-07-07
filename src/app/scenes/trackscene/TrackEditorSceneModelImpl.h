#pragma once

void TrackEditorScene::resetDraft()
{
    previewPlayer_->stop();
    chart_.clear();
    editingTrackId_.clear();
    savedTrackId_.clear();
    draftTitle_.fill('\0');
    draftArtist_.fill('\0');
    draftSource_.fill('\0');
    draftMapper_.fill('\0');
    draftAudioFile_.fill('\0');
    draftBpm_ = 120;
    snapIndex_ = 2;
    selectedPartIndex_ = 0;
    selectedNoteIndex_ = -1;
    zoom_ = 1.0f;
    durationSeconds_ = 90;
    timelineScrollX_ = 0.0f;
    timelineViewportWidth_ = 0.0f;
    transportCursorSeconds_ = 0.0;
    statusMessage_.clear();
    draftTuningName_.fill('\0');
    draftTuningStringCount_ = openchordix::track::kDefaultTrackStrings;
    draftTuningNotes_.clear();
    draftTuningTargetPartIndex_ = -1;
    nextEditorNoteId_ = 1;
    selectedNoteIds_.clear();
    selectionDragActive_ = false;
    selectionDragExceededThreshold_ = false;
    selectionStartTick_ = 0;
    selectionCurrentTick_ = 0;
    selectionStartLane_ = 0;
    selectionCurrentLane_ = 0;
    draggingNote_ = false;
    draggingNoteId_ = 0;
    dragStartMouseTick_ = 0;
    currentDragDeltaTicks_ = 0;
    draggedSelectionOriginalNotes_.clear();
    noteClipboard_ = {};
    undoStack_.clear();
    redoStack_.clear();
    noteInspectorEditActive_ = false;
    noteInspectorBeforeNotes_.clear();
    noteInspectorBeforeSelection_.clear();
    scrubbingTransport_ = false;
    scrubResumePlayback_ = false;
    timelineSyncPending_ = true;
    editorTool_ = EditorTool::Draw;
    importedSong_.reset();
    importDraftParts_.clear();
    importError_.clear();
    lastImportStatus_.clear();
    importProgress_ = {};
    importSourcePath_.clear();
    openImportPreview_ = false;
    importMergeMode_ = openchordix::track::imports::ImportMergeMode::Additive;
    importPlacement_ = {};
    importTimingPartIndex_ = 0;
    applyImportedTempo_ = true;
    draftParts_.clear();
    draftParts_.resize(1);
    initializeDraftPart(draftParts_[0], "default");
}

void TrackEditorScene::loadTrack(std::string_view trackId)
{
    resetDraft();
    const auto &tracks = catalog_->tracks();
    const auto it = std::find_if(
        tracks.begin(),
        tracks.end(),
        [&](const TrackInfo &track)
        {
            return track.id == trackId;
        });
    if (it == tracks.end())
    {
        statusMessage_ = "Track no longer exists.";
        return;
    }

    editingTrackId_ = it->id;
    savedTrackId_ = it->id;
    track_editor::copyText(draftTitle_, it->title);
    track_editor::copyText(draftArtist_, it->artist);
    track_editor::copyText(draftSource_, it->source);
    track_editor::copyText(draftMapper_, it->mapper);
    track_editor::copyText(draftAudioFile_, it->audioFile);
    draftBpm_ = std::max(1, it->bpm);
    durationSeconds_ = track_editor::parseLengthSeconds(it->length);

    draftParts_.clear();
    for (const TrackPart &part : it->parts)
    {
        draftParts_.push_back({});
        loadDraftPart(draftParts_.back(), part);
    }
    ensureSelectedPartIsValid();

    if (!chart_.load(currentChartPath(*it)))
    {
        statusMessage_ = chart_.lastError();
    }
    if (it->previewStartSeconds > 0.0 && chart_.previewStartSeconds() <= 0.0)
    {
        chart_.setPreviewStartSeconds(it->previewStartSeconds);
    }
    refreshDurationFromAudio();
    transportCursorSeconds_ = clampTransportCursor(chart_.previewStartSeconds());
    requestTimelineSync();
}

bool TrackEditorScene::saveDraft()
{
    TrackInfo track = buildTrackDraft();
    if (track.title.empty())
    {
        statusMessage_ = "Title is required.";
        return false;
    }
    if (track.artist.empty())
    {
        statusMessage_ = "Artist is required.";
        return false;
    }
    if (track.parts.empty())
    {
        statusMessage_ = "Add at least one instrument.";
        return false;
    }

    const std::string defaultPart = track.parts.front().name;
    for (TrackTabNote &note : chart_.notes())
    {
        if (note.part.empty())
        {
            note.part = defaultPart;
        }
        note.tick = std::max(0, quantizeTick(note.tick));
        note.duration = std::max(1, quantizeTick(note.duration));
        const auto partIt = std::find_if(
            track.parts.begin(),
            track.parts.end(),
            [&](const TrackPart &part)
            {
                return part.name == note.part;
            });
        const int stringCount =
            partIt != track.parts.end() ? openchordix::track::clampTrackStringCount(partIt->stringCount) : openchordix::track::kDefaultTrackStrings;
        note.stringIndex = std::clamp(note.stringIndex, 0, stringCount - 1);
        note.fret = std::max(0, note.fret);
    }
    sortNotes();

    bool success = false;
    if (editingTrackId_.empty())
    {
        success = catalog_->addTrack(track);
        if (success && !catalog_->tracks().empty())
        {
            editingTrackId_ = catalog_->tracks().back().id;
        }
    }
    else
    {
        success = catalog_->updateTrack(editingTrackId_, track);
    }

    if (!success)
    {
        statusMessage_ = "Failed to save track.";
        return false;
    }

    const auto &tracks = catalog_->tracks();
    const auto it = std::find_if(
        tracks.begin(),
        tracks.end(),
        [&](const TrackInfo &entry)
        {
            return entry.id == editingTrackId_;
        });
    if (it == tracks.end())
    {
        statusMessage_ = "Track saved, but could not be reloaded.";
        return false;
    }

    if (!chart_.save(currentChartPath(*it), *it))
    {
        statusMessage_ = chart_.lastError();
        return false;
    }

    savedTrackId_ = it->id;
    refreshDurationFromAudio();
    transportCursorSeconds_ = clampTransportCursor(chart_.previewStartSeconds());
    requestTimelineSync();
    statusMessage_ = "Saved.";
    return true;
}

void TrackEditorScene::sortNotes()
{
    std::sort(
        chart_.notes().begin(),
        chart_.notes().end(),
        [](const TrackTabNote &lhs, const TrackTabNote &rhs)
        {
            if (lhs.part != rhs.part)
            {
                return lhs.part < rhs.part;
            }
            if (lhs.tick != rhs.tick)
            {
                return lhs.tick < rhs.tick;
            }
            if (lhs.stringIndex != rhs.stringIndex)
            {
                return lhs.stringIndex < rhs.stringIndex;
            }
            return lhs.fret < rhs.fret;
        });
}

std::vector<TrackPart> TrackEditorScene::collectParts() const
{
    std::vector<TrackPart> parts;
    parts.reserve(draftParts_.size());
    for (const DraftPart &draft : draftParts_)
    {
        const TrackPart part = buildPartFromDraft(draft);
        if (!part.name.empty())
        {
            parts.push_back(part);
        }
    }
    return parts;
}

TrackPart TrackEditorScene::buildPartFromDraft(const DraftPart &draft) const
{
    TrackPart part;
    part.name = track_editor::trimCopy(draft.name.data());
    part.stringCount = openchordix::track::clampTrackStringCount(draft.stringCount);
    part.tuning = openchordix::track::normalizeTrackTuning(draft.tuning, part.name, part.stringCount);
    return part;
}

TrackPart TrackEditorScene::currentPart() const
{
    const auto parts = collectParts();
    if (parts.empty())
    {
        return TrackPart{};
    }
    const int index = std::clamp(selectedPartIndex_, 0, static_cast<int>(parts.size()) - 1);
    return parts[static_cast<size_t>(index)];
}

std::string TrackEditorScene::currentPartName() const
{
    const TrackPart part = currentPart();
    return part.name.empty() ? "default" : part.name;
}

int TrackEditorScene::currentPartStringCount() const
{
    return openchordix::track::clampTrackStringCount(currentPart().stringCount);
}

std::vector<std::string> TrackEditorScene::currentStringLabels() const
{
    const TrackPart part = currentPart();
    std::vector<std::string> labels = openchordix::track::normalizeTrackTuning(part.tuning, part.name, part.stringCount);
    for (std::string &label : labels)
    {
        label = openchordix::track::displayTuningNote(label);
    }
    return labels;
}

std::filesystem::path TrackEditorScene::trackRootDirectory() const
{
    return catalog_->storagePath().parent_path();
}

std::filesystem::path TrackEditorScene::resolveAudioBrowseDirectory() const
{
    const std::filesystem::path currentAudio = currentResolvedAudioPath();
    std::error_code ec;
    if (!currentAudio.empty() && std::filesystem::exists(currentAudio, ec))
    {
        return std::filesystem::is_directory(currentAudio, ec) ? currentAudio : currentAudio.parent_path();
    }
    return openchordix::track::resolveTrackRootDirectory(trackRootDirectory());
}

std::filesystem::path TrackEditorScene::currentChartPath(const TrackInfo &track) const
{
    return openchordix::track::resolveChartPath(track, trackRootDirectory());
}

std::filesystem::path TrackEditorScene::currentResolvedAudioPath() const
{
    TrackInfo track = buildTrackDraft();
    if (!editingTrackId_.empty())
    {
        track.id = editingTrackId_;
        if (track.directory.empty())
        {
            const auto &tracks = catalog_->tracks();
            const auto it = std::find_if(
                tracks.begin(),
                tracks.end(),
                [&](const TrackInfo &entry)
                {
                    return entry.id == editingTrackId_;
                });
            if (it != tracks.end())
            {
                track.directory = it->directory;
            }
        }
    }
    return openchordix::track::resolveAudioPath(track, trackRootDirectory());
}

void TrackEditorScene::applyChosenAudioPath(const std::filesystem::path &path)
{
    track_editor::copyText(draftAudioFile_, path.generic_string());
    refreshDurationFromAudio();
}

void TrackEditorScene::ensureSelectedPartIsValid()
{
    if (draftParts_.empty())
    {
        draftParts_.push_back({});
        initializeDraftPart(draftParts_.back(), "default");
    }
    selectedPartIndex_ = std::clamp(selectedPartIndex_, 0, static_cast<int>(draftParts_.size()) - 1);
}

void TrackEditorScene::initializeDraftPart(DraftPart &part, std::string_view name)
{
    part.name.fill('\0');
    track_editor::copyText(part.name, name);
    part.stringCount = openchordix::track::kDefaultTrackStrings;
    if (const auto *preset = tuningLibrary_.defaultPresetForPart(name, part.stringCount))
    {
        part.tuning = preset->notes;
    }
    else
    {
        part.tuning = openchordix::track::defaultTuningForPart(name, part.stringCount);
    }
}

void TrackEditorScene::loadDraftPart(DraftPart &draft, const TrackPart &part)
{
    track_editor::copyText(draft.name, part.name);
    draft.stringCount = openchordix::track::clampTrackStringCount(part.stringCount);
    draft.tuning = openchordix::track::normalizeTrackTuning(part.tuning, part.name, draft.stringCount);
    if (const auto *preset = tuningLibrary_.findByNotes(draft.tuning, true))
    {
        draft.tuning = preset->notes;
    }
}

void TrackEditorScene::setDraftPartStringCount(DraftPart &draft, int stringCount)
{
    draft.stringCount = openchordix::track::clampTrackStringCount(stringCount);
    if (const auto *preset = tuningLibrary_.defaultPresetForPart(draft.name.data(), draft.stringCount))
    {
        draft.tuning = preset->notes;
        return;
    }

    draft.tuning = openchordix::track::normalizeTrackTuning(draft.tuning, draft.name.data(), draft.stringCount);
}

void TrackEditorScene::applyPresetToDraftPart(DraftPart &draft, const openchordix::track::TuningPreset &preset)
{
    draft.stringCount = openchordix::track::clampTrackStringCount(static_cast<int>(preset.notes.size()));
    draft.tuning = preset.notes;
}

const openchordix::track::TuningPreset *TrackEditorScene::matchingPresetForDraftPart(const DraftPart &draft) const
{
    if (const auto *preset = tuningLibrary_.findByNotes(draft.tuning, false))
    {
        return preset;
    }
    return tuningLibrary_.findByNotes(draft.tuning, true);
}

void TrackEditorScene::openAddTuningDialog(int targetPartIndex)
{
    draftTuningName_.fill('\0');
    draftTuningTargetPartIndex_ = targetPartIndex;
    if (targetPartIndex >= 0 && targetPartIndex < static_cast<int>(draftParts_.size()))
    {
        DraftPart &draftPart = draftParts_[static_cast<size_t>(targetPartIndex)];
        draftTuningStringCount_ = draftPart.stringCount;
        if (const auto *preset = matchingPresetForDraftPart(draftPart))
        {
            draftTuningNotes_ = preset->notes;
        }
        else if (const auto *defaultPreset = tuningLibrary_.defaultPresetForPart(draftPart.name.data(), draftPart.stringCount))
        {
            draftTuningNotes_ = defaultPreset->notes;
        }
        else
        {
            draftTuningNotes_ = openchordix::track::normalizeTrackTuning(draftPart.tuning, draftPart.name.data(), draftPart.stringCount);
        }
    }
    else
    {
        draftTuningStringCount_ = openchordix::track::kDefaultTrackStrings;
        if (const auto *preset = tuningLibrary_.defaultPresetForPart("default", draftTuningStringCount_))
        {
            draftTuningNotes_ = preset->notes;
        }
        else
        {
            draftTuningNotes_ = openchordix::track::defaultTuningForPart("", draftTuningStringCount_);
        }
    }
    draftTuningNotes_.resize(static_cast<size_t>(draftTuningStringCount_));
    openAddTuning_ = true;
}

TrackInfo TrackEditorScene::buildTrackDraft() const
{
    TrackInfo track;
    track.id = editingTrackId_;
    track.title = track_editor::trimCopy(draftTitle_.data());
    track.artist = track_editor::trimCopy(draftArtist_.data());
    track.source = track_editor::trimCopy(draftSource_.data());
    track.mapper = track_editor::trimCopy(draftMapper_.data());
    track.audioFile = track_editor::trimCopy(draftAudioFile_.data());
    track.bpm = std::max(1, draftBpm_);
    track.length = track_editor::formatClock(songDurationSeconds());
    track.previewStartSeconds = chart_.previewStartSeconds();
    track.parts = collectParts();
    return track;
}
