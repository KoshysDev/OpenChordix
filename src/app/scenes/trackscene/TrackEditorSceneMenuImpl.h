#pragma once

#include <cmath>

void TrackEditorScene::openImportPicker(const std::vector<std::string> &extensions)
{
    importProgress_ = {
        openchordix::track::imports::ImportStage::SelectingFile,
        0.0f,
        "Choose a chart source file to import.",
    };
    importPicker_.setExtensions(extensions);
    importPicker_.setDirectory(resolveAudioBrowseDirectory());
    importPicker_.open();
}

void TrackEditorScene::renderFileMenu()
{
    if (!ImGui::BeginMenu("File"))
    {
        return;
    }

    if (ImGui::MenuItem("Save", "Ctrl+S"))
    {
        saveDraft();
    }
    ImGui::MenuItem("Save As...", nullptr, false, false);
    if (ImGui::MenuItem("Import..."))
    {
        openImportPicker({".mid", ".midi", ".gp3", ".gp4", ".gp5", ".gpx", ".gp"});
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Exit"))
    {
        previewPlayer_->stop();
        pendingAction_ = Action::Back;
    }
    ImGui::EndMenu();
}

void TrackEditorScene::renderEditMenu()
{
    if (!ImGui::BeginMenu("Edit"))
    {
        return;
    }

    ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
    ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
    ImGui::Separator();
    ImGui::MenuItem("Cut", "Ctrl+X", false, false);
    ImGui::MenuItem("Copy", "Ctrl+C", false, false);
    ImGui::MenuItem("Paste", "Ctrl+V", false, false);
    if (ImGui::MenuItem("Delete Selected Note", "Delete", false, selectedNoteIndex_ >= 0))
    {
        removeSelectedNote();
    }
    ImGui::Separator();
    ImGui::MenuItem("Select All", "Ctrl+A", false, false);
    ImGui::EndMenu();
}

void TrackEditorScene::renderSongMenu()
{
    if (!ImGui::BeginMenu("Song"))
    {
        return;
    }

    if (ImGui::MenuItem("Song Setup..."))
    {
        openSongSetup_ = true;
    }
    ImGui::MenuItem("Edit Metadata...", nullptr, false, false);
    const bool hasImportedTempo = importedSong_.has_value() && !importedSong_->tempos.empty();
    if (ImGui::MenuItem("Apply Imported Tempo", nullptr, false, hasImportedTempo))
    {
        draftBpm_ = std::max(1, static_cast<int>(std::lround(importedSong_->tempos.front().beatsPerMinute)));
        std::vector<openchordix::track::TempoEvent> events;
        events.reserve(importedSong_->tempos.size());
        for (const auto &tempo : importedSong_->tempos)
        {
            const long long scaled = static_cast<long long>(std::max(0, tempo.tick)) *
                                     static_cast<long long>(std::max(1, chart_.ticksPerBeat()));
            const long long source = static_cast<long long>(std::max(1, importedSong_->ticksPerBeat));
            events.push_back({static_cast<int>((scaled + source / 2) / source), tempo.beatsPerMinute, "import"});
        }
        chart_.setTempoEvents(std::move(events), static_cast<double>(draftBpm_));
        applyImportedTempo_ = true;
        statusMessage_ = "Imported tempo applied.";
    }
    ImGui::EndMenu();
}

void TrackEditorScene::renderTrackMenu()
{
    if (!ImGui::BeginMenu("Track"))
    {
        return;
    }

    if (ImGui::BeginMenu("Current Part"))
    {
        for (int index = 0; index < static_cast<int>(draftParts_.size()); ++index)
        {
            const std::string partName = track_editor::trimCopy(draftParts_[static_cast<size_t>(index)].name.data());
            const std::string label = partName.empty() ? "Untitled Part" : partName;
            if (ImGui::MenuItem(label.c_str(), nullptr, selectedPartIndex_ == index))
            {
                selectedPartIndex_ = index;
                selectedNoteIndex_ = -1;
            }
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Add Part"))
    {
        DraftPart part;
        initializeDraftPart(part, track_editor::defaultInstrumentName(draftParts_.size()));
        draftParts_.push_back(std::move(part));
        selectedPartIndex_ = static_cast<int>(draftParts_.size()) - 1;
        selectedNoteIndex_ = -1;
    }
    ImGui::MenuItem("Rename Part...", nullptr, false, false);

    const int activePartIndex = std::clamp(selectedPartIndex_, 0, static_cast<int>(draftParts_.size()) - 1);
    DraftPart &part = draftParts_[static_cast<size_t>(activePartIndex)];
    if (ImGui::BeginMenu("Tuning"))
    {
        const auto *selectedPreset = matchingPresetForDraftPart(part);
        for (const auto *preset : tuningLibrary_.presetsForStringCount(part.stringCount))
        {
            if (ImGui::MenuItem(preset->name.c_str(), nullptr, preset == selectedPreset))
            {
                applyPresetToDraftPart(part, *preset);
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("String Count"))
    {
        for (int stringCount = 1; stringCount <= openchordix::track::kMaxTrackStrings; ++stringCount)
        {
            const std::string label = std::to_string(stringCount);
            if (ImGui::MenuItem(label.c_str(), nullptr, stringCount == part.stringCount))
            {
                setDraftPartStringCount(part, stringCount);
            }
        }
        ImGui::EndMenu();
    }
    ImGui::EndMenu();
}

void TrackEditorScene::renderImportMenu()
{
    if (!ImGui::BeginMenu("Import"))
    {
        return;
    }

    if (ImGui::MenuItem("Import File..."))
    {
        openImportPicker({".mid", ".midi", ".gp3", ".gp4", ".gp5", ".gpx", ".gp"});
    }
    if (ImGui::MenuItem("Import MIDI..."))
    {
        openImportPicker({".mid", ".midi"});
    }
    if (ImGui::MenuItem("Import Guitar Pro..."))
    {
        openImportPicker({".gp3", ".gp4", ".gp5", ".gpx", ".gp"});
    }
    ImGui::Separator();
    const std::string importStatus = lastImportStatus_.empty()
                                         ? "Last Import Status: No import attempted"
                                         : "Last Import Status: " + lastImportStatus_;
    ImGui::MenuItem(importStatus.c_str(), nullptr, false, false);
    ImGui::EndMenu();
}

void TrackEditorScene::renderTransportMenu()
{
    if (!ImGui::BeginMenu("Transport"))
    {
        return;
    }

    if (ImGui::MenuItem(previewPlayer_->isPlaying() ? "Pause" : "Play", "Space"))
    {
        togglePreviewPlayback();
    }
    if (ImGui::MenuItem("Stop"))
    {
        stopPreviewToStart();
    }
    if (ImGui::MenuItem("Set Preview Here"))
    {
        chart_.setPreviewStartSeconds(clampTransportCursor(transportCursorSeconds_));
        statusMessage_ = "Preview timestamp updated.";
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Go to Preview Start"))
    {
        previewPlayer_->stop();
        transportCursorSeconds_ = clampTransportCursor(chart_.previewStartSeconds());
        requestTimelineSync();
    }
    if (ImGui::MenuItem("Go to Start"))
    {
        previewPlayer_->stop();
        transportCursorSeconds_ = 0.0;
        requestTimelineSync();
    }
    ImGui::EndMenu();
}

void TrackEditorScene::renderViewMenu()
{
    if (!ImGui::BeginMenu("View"))
    {
        return;
    }

    if (ImGui::MenuItem("Zoom In"))
    {
        zoom_ = std::min(track_editor::kMaxZoom, zoom_ * 1.12f);
    }
    if (ImGui::MenuItem("Zoom Out"))
    {
        zoom_ = std::max(track_editor::kMinZoom, zoom_ / 1.12f);
    }
    if (ImGui::MenuItem("Reset Zoom"))
    {
        zoom_ = 1.0f;
    }
    ImGui::Separator();
    ImGui::MenuItem("Timing Diagnostics", nullptr, &showTimingDiagnostics_);
    ImGui::Separator();
    if (ImGui::BeginMenu("Snap"))
    {
        for (int index = 0; index < static_cast<int>(track_editor::kSnapLabels.size()); ++index)
        {
            if (ImGui::MenuItem(track_editor::kSnapLabels[static_cast<size_t>(index)], nullptr, snapIndex_ == index))
            {
                snapIndex_ = index;
            }
        }
        ImGui::EndMenu();
    }
    ImGui::EndMenu();
}

void TrackEditorScene::renderToolsMenu()
{
    if (!ImGui::BeginMenu("Tools"))
    {
        return;
    }

    if (ImGui::MenuItem("Select Tool", nullptr, editorTool_ == EditorTool::Select))
    {
        editorTool_ = EditorTool::Select;
    }
    if (ImGui::MenuItem("Draw Note Tool", nullptr, editorTool_ == EditorTool::Draw))
    {
        editorTool_ = EditorTool::Draw;
    }
    if (ImGui::MenuItem("Erase Tool", nullptr, editorTool_ == EditorTool::Erase))
    {
        editorTool_ = EditorTool::Erase;
    }
    if (ImGui::MenuItem("Move Tool", nullptr, editorTool_ == EditorTool::Move))
    {
        editorTool_ = EditorTool::Move;
    }
    ImGui::EndMenu();
}

void TrackEditorScene::renderHelpMenu()
{
    if (!ImGui::BeginMenu("Help"))
    {
        return;
    }

    ImGui::MenuItem("Keyboard Shortcuts", nullptr, false, false);
    ImGui::MenuItem("About OpenChordix", nullptr, false, false);
    ImGui::EndMenu();
}
