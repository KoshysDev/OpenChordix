#pragma once

void TrackEditorScene::drawSongSetupModal()
{
    if (openSongSetup_)
    {
        ImGui::OpenPopup("Song Setup");
        openSongSetup_ = false;
    }

    ImGui::SetNextWindowSize(ImVec2(980.0f, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Song Setup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(track_editor::kAccent, "Song Details");
        ImGui::Separator();

        ImGui::SetNextItemWidth(320.0f);
        ImGui::InputTextWithHint("Title", "Song title", draftTitle_.data(), draftTitle_.size());
        ImGui::SetNextItemWidth(320.0f);
        ImGui::InputTextWithHint("Artist", "Artist", draftArtist_.data(), draftArtist_.size());
        ImGui::SetNextItemWidth(320.0f);
        ImGui::InputTextWithHint("Source", "Album / source", draftSource_.data(), draftSource_.size());
        ImGui::SetNextItemWidth(320.0f);
        ImGui::InputTextWithHint("Mapper", "Mapper", draftMapper_.data(), draftMapper_.size());
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("BPM", &draftBpm_);
        draftBpm_ = std::max(1, draftBpm_);

        ImGui::Spacing();
        ImGui::TextColored(track_editor::kAccent, "Audio");
        ImGui::Separator();
        ImGui::SetNextItemWidth(520.0f);
        ImGui::InputTextWithHint("Audio File", "Audio path", draftAudioFile_.data(), draftAudioFile_.size());
        ImGui::SameLine();
        if (ImGui::Button("Browse...", ImVec2(96.0f, 0.0f)))
        {
            audioPicker_.setDirectory(resolveAudioBrowseDirectory());
            audioPicker_.open();
        }
        ImGui::TextDisabled("Selected audio is copied into the song folder on save.");
        ImGui::TextDisabled("Detected length: %s", track_editor::formatClock(songDurationSeconds()).c_str());

        ImGui::Spacing();
        ImGui::TextColored(track_editor::kAccent, "Instruments");
        ImGui::Separator();
        std::optional<size_t> removeIndex;
        for (size_t i = 0; i < draftParts_.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            DraftPart &draftPart = draftParts_[i];

            ImGui::SetNextItemWidth(220.0f);
            ImGui::InputTextWithHint("##part_name", "Instrument", draftPart.name.data(), draftPart.name.size());
            ImGui::SameLine();
            ImGui::TextDisabled("Strings");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            int stringCount = draftPart.stringCount;
            if (track_editor::drawStringCountCombo("##part_strings", stringCount))
            {
                setDraftPartStringCount(draftPart, stringCount);
            }

            ImGui::SameLine();
            const auto tunings = tuningLibrary_.presetsForStringCount(draftPart.stringCount);
            const auto *matchingPreset = matchingPresetForDraftPart(draftPart);
            const std::string tuningLabel = matchingPreset != nullptr ? matchingPreset->name : "Custom";
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::BeginCombo("##part_tuning", tuningLabel.c_str()))
            {
                for (const auto *preset : tunings)
                {
                    const bool selected = matchingPreset == preset;
                    if (ImGui::Selectable(preset->name.c_str(), selected))
                    {
                        applyPresetToDraftPart(draftPart, *preset);
                    }
                    if (!preset->notes.empty())
                    {
                        ImGui::SameLine();
                        ImGui::TextDisabled("%s", openchordix::track::tuningNotesSummary(preset->notes).c_str());
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Button("Add Tuning", ImVec2(104.0f, 0.0f)))
            {
                openAddTuningDialog(static_cast<int>(i));
            }
            if (draftParts_.size() > 1)
            {
                ImGui::SameLine();
                if (ImGui::Button("Delete", ImVec2(84.0f, 0.0f)))
                {
                    removeIndex = i;
                }
            }

            ImGui::TextDisabled("%s", openchordix::track::tuningNotesSummary(draftPart.tuning).c_str());
            ImGui::Spacing();
            ImGui::PopID();
        }

        if (removeIndex.has_value())
        {
            const std::string removedPart = track_editor::trimCopy(draftParts_[*removeIndex].name.data());
            draftParts_.erase(draftParts_.begin() + static_cast<std::ptrdiff_t>(*removeIndex));
            ensureSelectedPartIsValid();
            for (TrackTabNote &note : chart_.notes())
            {
                if (note.part == removedPart)
                {
                    note.part = currentPartName();
                }
            }
        }

        if (ui_.button("Add Instrument", ImVec2(160.0f, 0.0f)))
        {
            DraftPart newPart;
            initializeDraftPart(newPart, track_editor::defaultInstrumentName(draftParts_.size()));
            draftParts_.push_back(std::move(newPart));
            selectedPartIndex_ = static_cast<int>(draftParts_.size()) - 1;
        }

        ImGui::Spacing();
        if (ImGui::Button("Done", ImVec2(140.0f, 0.0f)))
        {
            refreshDurationFromAudio();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save", ImVec2(140.0f, 0.0f)))
        {
            refreshDurationFromAudio();
            saveDraft();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(140.0f, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void TrackEditorScene::drawAddTuningModal()
{
    if (openAddTuning_)
    {
        ImGui::OpenPopup("Add Tuning");
        openAddTuning_ = false;
    }

    if (ImGui::BeginPopupModal("Add Tuning", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SetNextItemWidth(280.0f);
        ImGui::InputTextWithHint("Name", "Tuning name", draftTuningName_.data(), draftTuningName_.size());

        ImGui::SetNextItemWidth(120.0f);
        const int previousStringCount = draftTuningStringCount_;
        track_editor::drawStringCountCombo("Strings", draftTuningStringCount_);
        if (draftTuningTargetPartIndex_ >= 0 &&
            draftTuningTargetPartIndex_ < static_cast<int>(draftParts_.size()) &&
            previousStringCount != draftTuningStringCount_)
        {
            const auto *preset = tuningLibrary_.defaultPresetForPart(draftParts_[static_cast<size_t>(draftTuningTargetPartIndex_)].name.data(),
                                                                     draftTuningStringCount_);
            draftTuningNotes_ = preset != nullptr
                                    ? preset->notes
                                    : openchordix::track::defaultTuningForPart(draftParts_[static_cast<size_t>(draftTuningTargetPartIndex_)].name.data(),
                                                                               draftTuningStringCount_);
        }
        draftTuningNotes_.resize(static_cast<size_t>(draftTuningStringCount_));

        ImGui::TextDisabled("Top to bottom notes");
        const auto &options = track_editor::tuningNoteOptions();
        for (int stringIndex = 0; stringIndex < draftTuningStringCount_; ++stringIndex)
        {
            if (static_cast<int>(draftTuningNotes_.size()) <= stringIndex)
            {
                draftTuningNotes_.resize(static_cast<size_t>(draftTuningStringCount_), options.front());
            }

            ImGui::PushID(stringIndex);
            const std::string current = openchordix::track::normalizeTuningNote(draftTuningNotes_[static_cast<size_t>(stringIndex)]);
            const std::string label = "String " + std::to_string(stringIndex + 1);
            ImGui::SetNextItemWidth(130.0f);
            if (ImGui::BeginCombo(label.c_str(), current.c_str()))
            {
                for (const std::string &option : options)
                {
                    const bool selected = current == option;
                    if (ImGui::Selectable(option.c_str(), selected))
                    {
                        draftTuningNotes_[static_cast<size_t>(stringIndex)] = option;
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if ((stringIndex + 1) % 4 != 0 && stringIndex + 1 < draftTuningStringCount_)
            {
                ImGui::SameLine();
            }
            ImGui::PopID();
        }

        ImGui::Spacing();
        if (ImGui::Button("Add", ImVec2(140.0f, 0.0f)))
        {
            const std::string tuningName = track_editor::trimCopy(draftTuningName_.data());
            if (tuningName.empty())
            {
                statusMessage_ = "Tuning name is required.";
            }
            else
            {
                openchordix::track::TuningPreset preset{tuningName, draftTuningNotes_};
                if (tuningLibrary_.addPreset(preset))
                {
                    const auto *savedPreset = tuningLibrary_.findByNotes(preset.notes, false);
                    if (savedPreset == nullptr)
                    {
                        savedPreset = tuningLibrary_.findByNotes(preset.notes, true);
                    }
                    if (draftTuningTargetPartIndex_ >= 0 && draftTuningTargetPartIndex_ < static_cast<int>(draftParts_.size()))
                    {
                        applyPresetToDraftPart(
                            draftParts_[static_cast<size_t>(draftTuningTargetPartIndex_)],
                            savedPreset != nullptr ? *savedPreset : preset);
                    }
                    statusMessage_ = "Tuning saved.";
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    statusMessage_ = "Failed to save tuning.";
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(140.0f, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
