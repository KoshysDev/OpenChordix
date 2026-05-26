#pragma once

#include <cmath>
#include <numeric>

namespace
{
    const char *importStageLabel(openchordix::track::imports::ImportStage stage)
    {
        using openchordix::track::imports::ImportStage;
        switch (stage)
        {
        case ImportStage::SelectingFile:
            return "Selecting file";
        case ImportStage::Loading:
            return "Loading";
        case ImportStage::Parsing:
            return "Parsing";
        case ImportStage::Converting:
            return "Converting";
        case ImportStage::ReadyToPreview:
            return "Ready to preview";
        case ImportStage::Applying:
            return "Applying";
        case ImportStage::Complete:
            return "Complete";
        case ImportStage::Failed:
            return "Failed";
        default:
            return "Idle";
        }
    }
}

void TrackEditorScene::loadImportSource(const std::filesystem::path &path)
{
    importError_.clear();
    importDetectedFormat_.clear();
    importedSong_.reset();
    importDraftParts_.clear();
    importSourcePath_ = path;
    importMergeMode_ = openchordix::track::imports::ImportMergeMode::Additive;
    importPlacement_ = {};
    importTimingPartIndex_ = 0;
    importProgress_ = {
        openchordix::track::imports::ImportStage::Loading,
        0.15f,
        "Loading " + path.filename().string() + "...",
    };

    importProgress_ = {
        openchordix::track::imports::ImportStage::Parsing,
        0.4f,
        "Parsing chart source and detecting tracks...",
    };
    auto result = importerRegistry_.importFile(path);
    if (!result)
    {
        importError_ = result.error().message;
        importDetectedFormat_ = result.error().detectedFormat;
        lastImportStatus_ = "Failed - " + path.filename().string();
        importProgress_ = {
            openchordix::track::imports::ImportStage::Failed,
            0.4f,
            importError_,
        };
        openImportPreview_ = true;
        return;
    }

    importProgress_ = {
        openchordix::track::imports::ImportStage::Converting,
        0.75f,
        "Preparing importable parts and tablature preview...",
    };
    importedSong_ = std::move(result.value());
    importDetectedFormat_ = importedSong_->detectedFormat;
    lastImportStatus_ = "Loaded " + path.filename().string();
    applyImportedTempo_ = chart_.notes().empty() && !importedSong_->tempos.empty();
    importDraftParts_.reserve(importedSong_->parts.size());
    for (const openchordix::track::imports::ImportedPart &source : importedSong_->parts)
    {
        ImportPartDraft draft;
        draft.enabled = source.importByDefault && !source.notes.empty();
        track_editor::copyText(draft.name, source.name);
        draft.stringCount = openchordix::track::clampTrackStringCount(source.suggestedStringCount);
        draft.tuning = openchordix::track::normalizeTrackTuning(
            source.suggestedTuning, source.name, draft.stringCount);
        importDraftParts_.push_back(std::move(draft));
    }
    importProgress_ = {
        openchordix::track::imports::ImportStage::ReadyToPreview,
        1.0f,
        "Review the detected parts, then apply the import.",
    };
    openImportPreview_ = true;
}

void TrackEditorScene::applyImportPreview()
{
    if (!importedSong_.has_value())
    {
        return;
    }

    importProgress_ = {
        openchordix::track::imports::ImportStage::Applying,
        0.92f,
        "Applying selected parts to the chart...",
    };
    std::vector<openchordix::track::imports::SelectedImportPart> selections;
    selections.reserve(importDraftParts_.size());
    for (size_t index = 0; index < importDraftParts_.size(); ++index)
    {
        const ImportPartDraft &draft = importDraftParts_[index];
        selections.push_back({
            index,
            draft.enabled,
            track_editor::trimCopy(draft.name.data()),
            draft.stringCount,
            draft.tuning,
        });
    }

    auto placement = importPlacement_;
    placement.currentCursorTick = std::max(
        0, static_cast<int>(std::lround(timelineTickFromSeconds(displayedCursorSeconds()))));
    const auto summary = openchordix::track::imports::applyImportedParts(
        chart_, *importedSong_, selections, importMergeMode_, placement);
    for (const TrackPart &part : summary.appliedParts)
    {
        const auto existing = std::find_if(
            draftParts_.begin(), draftParts_.end(),
            [&](const DraftPart &draft)
            { return track_editor::trimCopy(draft.name.data()) == part.name; });
        if (existing != draftParts_.end())
        {
            loadDraftPart(*existing, part);
            continue;
        }

        draftParts_.push_back({});
        loadDraftPart(draftParts_.back(), part);
    }

    if (applyImportedTempo_ && !importedSong_->tempos.empty())
    {
        draftBpm_ = std::max(1, static_cast<int>(std::lround(importedSong_->tempos.front().beatsPerMinute)));
    }
    if (importedSong_->durationSeconds > 0.0)
    {
        durationSeconds_ = std::max(durationSeconds_, static_cast<int>(std::ceil(importedSong_->durationSeconds)));
    }
    ensureSelectedPartIsValid();
    sortNotes();
    requestTimelineSync();
    statusMessage_ = "Imported " + std::to_string(summary.addedNotes) + " notes from " +
                     importedSong_->sourcePath.filename().string() + " at tick " +
                     std::to_string(summary.placementOffsetTicks) + ".";
    lastImportStatus_ = statusMessage_;
    importProgress_ = {
        openchordix::track::imports::ImportStage::Complete,
        1.0f,
        statusMessage_,
    };
    importedSong_.reset();
    importDraftParts_.clear();
    importError_.clear();
    importDetectedFormat_.clear();
}

void TrackEditorScene::drawImportPreviewModal()
{
    if (openImportPreview_)
    {
        ImGui::OpenPopup("Import Chart Preview");
        openImportPreview_ = false;
    }

    ImGui::SetNextWindowSize(ImVec2(1120.0f, 640.0f), ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal("Import Chart Preview", nullptr, ImGuiWindowFlags_NoResize))
    {
        return;
    }

    ImGui::TextColored(track_editor::kAccent, "Import Chart");
    ImGui::Separator();
    const std::string sourceName = importSourcePath_.empty() ? "No file selected" : importSourcePath_.string();
    ImGui::Text("File: %s", sourceName.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("| Stage: %s", importStageLabel(importProgress_.stage));
    if (!importDetectedFormat_.empty())
    {
        ImGui::TextDisabled("Detected format: %s", importDetectedFormat_.c_str());
    }
    ImGui::ProgressBar(std::clamp(importProgress_.fraction, 0.0f, 1.0f), ImVec2(-1.0f, 0.0f));
    ImGui::TextDisabled("%s", importProgress_.status.c_str());
    ImGui::Spacing();

    if (!importError_.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.38f, 1.0f), "Import failed");
        ImGui::TextWrapped("%s", importError_.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Choose Another File", ImVec2(168.0f, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
            openImportPicker({".mid", ".midi", ".gp3", ".gp4", ".gp5", ".gpx", ".gp"});
        }
        ImGui::SameLine();
        if (ImGui::Button("Retry", ImVec2(100.0f, 0.0f)))
        {
            loadImportSource(importSourcePath_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
        {
            importError_.clear();
            importDetectedFormat_.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return;
    }

    if (!importedSong_.has_value())
    {
        ImGui::TextDisabled("No chart file has been loaded.");
        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return;
    }

    const auto &song = *importedSong_;
    const size_t noteCount = std::accumulate(
        song.parts.begin(), song.parts.end(), size_t{0},
        [](size_t value, const openchordix::track::imports::ImportedPart &part)
        { return value + part.notes.size(); });
    const char *formatLabel =
        song.format == openchordix::track::imports::ImportFormat::Midi ? "MIDI" : "Guitar Pro";
    ImGui::Text("%s", formatLabel);
    const std::string metadata = song.title.empty()
                                     ? "Metadata not provided"
                                     : song.title + (song.artist.empty() ? "" : " - " + song.artist);
    ImGui::TextDisabled("%s%s%s",
                        metadata.c_str(),
                        song.album.empty() ? "" : "  |  Album: ",
                        song.album.c_str());
    ImGui::TextDisabled("%zu detected parts  |  %zu notes  |  %zu measures  |  %.2f seconds  |  %d ticks/beat",
                        song.parts.size(), noteCount, song.measures.size(), song.durationSeconds, song.ticksPerBeat);
    if (song.format == openchordix::track::imports::ImportFormat::Midi)
    {
        ImGui::TextColored(track_editor::kMuted,
                           "MIDI has pitch timing but no tablature fingering. Imported notes start at string 1, fret 0 for manual mapping.");
    }
    else
    {
        const bool gpifPackage = song.detectedFormat.find("zip package") != std::string::npos;
        ImGui::TextColored(track_editor::kMuted,
                           gpifPackage
                               ? "GPIF tablature keeps stored string, fret, rhythmic duration, and supported note techniques."
                               : "GP5 tablature keeps stored string, fret, rhythmic duration, and supported note techniques.");
    }

    ImGui::Spacing();
    int mergeMode = importMergeMode_ == openchordix::track::imports::ImportMergeMode::Additive ? 0 : 1;
    ImGui::RadioButton("Add to current chart", &mergeMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Replace all current notes", &mergeMode, 1);
    importMergeMode_ = mergeMode == 0 ? openchordix::track::imports::ImportMergeMode::Additive
                                      : openchordix::track::imports::ImportMergeMode::ReplaceAllNotes;
    if (!song.tempos.empty())
    {
        ImGui::SameLine();
        ImGui::Checkbox("Use imported tempo", &applyImportedTempo_);
        ImGui::SameLine();
        ImGui::TextDisabled("(%.1f BPM)", song.tempos.front().beatsPerMinute);
    }

    ImGui::Spacing();
    ImGui::Text("Placement");
    int placementMode = static_cast<int>(importPlacement_.mode);
    ImGui::RadioButton("Start at tick 0", &placementMode,
                       static_cast<int>(openchordix::track::imports::ImportPlacement::StartAtBeginning));
    ImGui::SameLine();
    ImGui::RadioButton("Current cursor", &placementMode,
                       static_cast<int>(openchordix::track::imports::ImportPlacement::CurrentCursor));
    ImGui::SameLine();
    ImGui::RadioButton("Append after chart", &placementMode,
                       static_cast<int>(openchordix::track::imports::ImportPlacement::AppendAfterExisting));
    ImGui::SameLine();
    ImGui::RadioButton("Custom tick", &placementMode,
                       static_cast<int>(openchordix::track::imports::ImportPlacement::CustomTickOffset));
    importPlacement_.mode = static_cast<openchordix::track::imports::ImportPlacement>(placementMode);
    if (importPlacement_.mode == openchordix::track::imports::ImportPlacement::CustomTickOffset)
    {
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputInt("Tick offset", &importPlacement_.customTickOffset);
        importPlacement_.customTickOffset = std::max(0, importPlacement_.customTickOffset);
    }
    int shownOffset = 0;
    if (importPlacement_.mode == openchordix::track::imports::ImportPlacement::CurrentCursor)
    {
        shownOffset = std::max(0, static_cast<int>(std::lround(timelineTickFromSeconds(displayedCursorSeconds()))));
    }
    else if (importPlacement_.mode == openchordix::track::imports::ImportPlacement::AppendAfterExisting)
    {
        shownOffset = chart_.timelineEndTick();
    }
    else if (importPlacement_.mode == openchordix::track::imports::ImportPlacement::CustomTickOffset)
    {
        shownOffset = importPlacement_.customTickOffset;
    }
    ImGui::TextDisabled("Imported source ticks will be offset by %d chart ticks.", shownOffset);

    const auto timingDebug = openchordix::track::imports::buildTimingDebugSummary(song, 16, 32);
    if (ImGui::CollapsingHeader("Timing debug"))
    {
        ImGui::TextDisabled("TPB: %d  |  Tempo: %.1f  |  Measures: %zu",
                            timingDebug.ticksPerBeat,
                            timingDebug.initialTempo.value_or(0.0),
                            timingDebug.measureCount);
        if (!timingDebug.parts.empty())
        {
            importTimingPartIndex_ = std::clamp(importTimingPartIndex_, 0,
                                                static_cast<int>(timingDebug.parts.size()) - 1);
            const auto &current = timingDebug.parts[static_cast<std::size_t>(importTimingPartIndex_)];
            if (ImGui::BeginCombo("Part##timing_part", current.name.c_str()))
            {
                for (std::size_t index = 0; index < timingDebug.parts.size(); ++index)
                {
                    const bool selected = static_cast<int>(index) == importTimingPartIndex_;
                    if (ImGui::Selectable(timingDebug.parts[index].name.c_str(), selected))
                    {
                        importTimingPartIndex_ = static_cast<int>(index);
                    }
                }
                ImGui::EndCombo();
            }
            const auto &partDebug = timingDebug.parts[static_cast<std::size_t>(importTimingPartIndex_)];
            ImGui::TextDisabled("First note tick: %d  |  First note measure: %d  |  First non-empty measure: %d",
                                partDebug.firstNoteTick.value_or(-1),
                                partDebug.firstNoteMeasureIndex + 1,
                                partDebug.firstNonEmptyMeasureIndex + 1);
            if (ImGui::BeginChild("timing_debug_details", ImVec2(0.0f, 155.0f), true))
            {
                ImGui::Text("First note events: measure | within | absolute | duration | string | fret | techniques");
                for (const auto &note : partDebug.notes)
                {
                    ImGui::Text("%d | %d | %d | %d | %d | %d | %s",
                                note.measureIndex + 1, note.tickWithinMeasure, note.absoluteTick,
                                note.durationTicks, note.stringIndex + 1, note.fret, note.techniques.c_str());
                }
                ImGui::Separator();
                ImGui::Text("Measure map: # | start | length | signature");
                for (const auto &measure : timingDebug.measures)
                {
                    ImGui::Text("%d%s | %d | %d | %d/%d",
                                measure.number, measure.pickup ? " (pickup)" : "",
                                measure.startTick, measure.durationTicks,
                                measure.numerator, measure.denominator);
                }
            }
            ImGui::EndChild();
        }
    }

    ImGui::Spacing();
    if (ImGui::BeginTable("import_parts", 9,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp,
                          ImVec2(0.0f, 234.0f)))
    {
        ImGui::TableSetupColumn("Import", ImGuiTableColumnFlags_WidthFixed, 56.0f);
        ImGui::TableSetupColumn("Track #", ImGuiTableColumnFlags_WidthFixed, 58.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.4f);
        ImGui::TableSetupColumn("Type / Instrument", ImGuiTableColumnFlags_WidthStretch, 1.1f);
        ImGui::TableSetupColumn("Strings", ImGuiTableColumnFlags_WidthFixed, 82.0f);
        ImGui::TableSetupColumn("Tuning", ImGuiTableColumnFlags_WidthStretch, 1.1f);
        ImGui::TableSetupColumn("Notes", ImGuiTableColumnFlags_WidthFixed, 54.0f);
        ImGui::TableSetupColumn("Measures", ImGuiTableColumnFlags_WidthFixed, 68.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.25f);
        ImGui::TableHeadersRow();

        for (size_t index = 0; index < importDraftParts_.size(); ++index)
        {
            const auto &source = song.parts[index];
            ImportPartDraft &draft = importDraftParts_[index];
            ImGui::PushID(static_cast<int>(index));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Checkbox("##enabled", &draft.enabled);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", index + 1);

            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint("##name", "Imported part", draft.name.data(), draft.name.size());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", openchordix::track::imports::partTypeLabel(source.type));
            if (!source.instrumentName.empty())
            {
                ImGui::TextDisabled("%s", source.instrumentName.c_str());
            }

            ImGui::TableSetColumnIndex(4);
            if (source.detectedStringCount <= 0)
            {
                ImGui::TextDisabled("0");
            }
            else
            {
                ImGui::SetNextItemWidth(-1.0f);
                int stringCount = draft.stringCount;
                if (track_editor::drawStringCountCombo("##strings", stringCount))
                {
                    draft.stringCount = stringCount;
                    if (const auto *preset = tuningLibrary_.defaultPresetForPart(draft.name.data(), stringCount))
                    {
                        draft.tuning = preset->notes;
                    }
                    else
                    {
                        draft.tuning = openchordix::track::defaultTuningForPart(draft.name.data(), stringCount);
                    }
                }
            }

            ImGui::TableSetColumnIndex(5);
            if (source.detectedStringCount <= 0)
            {
                ImGui::TextDisabled("N/A");
            }
            else
            {
                const auto *matchingPreset = tuningLibrary_.findByNotes(draft.tuning, true);
                const std::string tuningLabel = matchingPreset != nullptr
                                                    ? matchingPreset->name
                                                    : openchordix::track::tuningNotesSummary(draft.tuning);
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::BeginCombo("##tuning", tuningLabel.c_str()))
                {
                    for (const auto *preset : tuningLibrary_.presetsForStringCount(draft.stringCount))
                    {
                        const bool selected = matchingPreset == preset;
                        if (ImGui::Selectable(preset->name.c_str(), selected))
                        {
                            draft.tuning = preset->notes;
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%zu", source.notes.size());

            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%zu", source.measures.size());

            ImGui::TableSetColumnIndex(8);
            ImGui::TextWrapped("%s", source.status.c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (ImGui::Button("Apply Import", ImVec2(140.0f, 0.0f)))
    {
        applyImportPreview();
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }
    ImGui::SameLine();
    if (ImGui::Button("Choose Another File", ImVec2(168.0f, 0.0f)))
    {
        importedSong_.reset();
        importDraftParts_.clear();
        importError_.clear();
        importDetectedFormat_.clear();
        ImGui::CloseCurrentPopup();
        openImportPicker({".mid", ".midi", ".gp3", ".gp4", ".gp5", ".gpx", ".gp"});
    }
    ImGui::SameLine();
    if (ImGui::Button("Retry", ImVec2(100.0f, 0.0f)))
    {
        loadImportSource(importSourcePath_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
    {
        importedSong_.reset();
        importDraftParts_.clear();
        importError_.clear();
        importDetectedFormat_.clear();
        importProgress_ = {};
        importSourcePath_.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
