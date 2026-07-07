#pragma once

void TrackEditorScene::drawBottomBar(const ImVec2 &screen)
{
    const float height = track_editor::kBottomBarHeight;
    const float headerY = 12.0f;
    const float fieldY = 34.0f;
    const float deleteButtonY = 42.0f;
    const float comboY = 88.0f;
    const float chipRow1Y = 124.0f;
    const float chipRow2Y = 156.0f;
    const float transportStripHeight = 34.0f;
    const float transportStripY = height - transportStripHeight - 10.0f;
    const float transportControlsY = transportStripY - 44.0f;

    ImGui::SetCursorPos(ImVec2(12.0f, screen.y - height - 12.0f));
    ImGui::BeginChild("track_editor_bottom",
                      ImVec2(screen.x - 24.0f, height),
                      false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 max(min.x + screen.x - 24.0f, min.y + height);
    track_editor::drawPanelFrame(dl, min, max, track_editor::kPanel);
    const double cursorSeconds = displayedCursorSeconds();

    TrackTabNote *selectedNote =
        selectedNoteIndex_ >= 0 && selectedNoteIndex_ < static_cast<int>(chart_.notes().size()) ? &chart_.notes()[selectedNoteIndex_] : nullptr;

    const auto notesEqual = [](const std::vector<TrackTabNote> &left, const std::vector<TrackTabNote> &right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.size(); ++index)
        {
            const TrackTabNote &a = left[index];
            const TrackTabNote &b = right[index];
            if (a.part != b.part || a.tick != b.tick || a.duration != b.duration ||
                a.stringIndex != b.stringIndex || a.fret != b.fret ||
                a.noteType != b.noteType || a.slideType != b.slideType ||
                a.harmonicType != b.harmonicType || a.pluckStyle != b.pluckStyle ||
                a.hammerOn != b.hammerOn || a.pullOff != b.pullOff ||
                a.bend != b.bend || a.vibrato != b.vibrato ||
                a.palmMute != b.palmMute || a.letRing != b.letRing ||
                a.staccato != b.staccato || a.tremoloPicking != b.tremoloPicking ||
                a.trill != b.trill || a.accent != b.accent ||
                a.heavyAccent != b.heavyAccent || a.editorId != b.editorId)
            {
                return false;
            }
        }
        return true;
    };

    if (selectedNote != nullptr)
    {
        const std::vector<TrackTabNote> inspectorFrameBeforeNotes = chart_.notes();
        const std::vector<openchordix::track::editor::EditorNoteId> inspectorFrameBeforeSelection = selectedNoteIds_;
        const std::vector<std::string> stringLabels = currentStringLabels();
        const int stringIndex = std::clamp(selectedNote->stringIndex, 0, static_cast<int>(stringLabels.size()) - 1);
        const int snap = snapTickSize();
        int lengthSteps = std::max(1, static_cast<int>(std::round(static_cast<float>(selectedNote->duration) / static_cast<float>(snap))));

        ImGui::SetCursorPos(ImVec2(14.0f, headerY));
        ImGui::TextColored(track_editor::kAccent, "Selected Note");
        ImGui::SameLine();
        ImGui::TextDisabled("%s  |  String %d (%s)  |  Tick %d",
                            currentPartName().c_str(),
                            stringIndex + 1,
                            stringLabels[static_cast<size_t>(stringIndex)].c_str(),
                            selectedNote->tick);

        track_editor::drawStepperField("Fret", "bottom_note_fret", selectedNote->fret, 0, 48, 1, "", ImVec2(14.0f, fieldY), 92.0f);
        track_editor::drawStepperSlider("Length", "bottom_note_length", lengthSteps, 1, 64, 1, "%dx snap", ImVec2(220.0f, fieldY), 220.0f);
        selectedNote->fret = std::max(0, selectedNote->fret);
        selectedNote->duration = std::max(1, quantizeTick(lengthSteps * snap));

        ImGui::SetCursorPos(ImVec2(std::max(640.0f, screen.x - 184.0f), deleteButtonY));
        if (ui_.button("Delete Note", ImVec2(132.0f, 34.0f)))
        {
            removeSelectedNote();
            selectedNote = nullptr;
        }

        if (selectedNote != nullptr)
        {
            ImGui::SetCursorPos(ImVec2(14.0f, comboY));
            int noteTypeIndex = track_editor::comboIndexFromValue(track_editor::kNoteTypeLabels, selectedNote->noteType);
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::Combo("##bottom_note_type", &noteTypeIndex, track_editor::kNoteTypeLabels.data(), static_cast<int>(track_editor::kNoteTypeLabels.size())))
            {
                selectedNote->noteType = track_editor::kNoteTypeLabels[static_cast<size_t>(noteTypeIndex)];
            }
            ImGui::SameLine();
            int slideTypeIndex = track_editor::comboIndexFromValue(track_editor::kSlideTypeLabels, selectedNote->slideType.empty() ? "none" : selectedNote->slideType);
            ImGui::SetNextItemWidth(210.0f);
            if (ImGui::Combo("##bottom_note_slide", &slideTypeIndex, track_editor::kSlideTypeLabels.data(), static_cast<int>(track_editor::kSlideTypeLabels.size())))
            {
                track_editor::assignComboValue(selectedNote->slideType, track_editor::kSlideTypeLabels, slideTypeIndex);
            }
            ImGui::SameLine();
            int harmonicTypeIndex = track_editor::comboIndexFromValue(track_editor::kHarmonicTypeLabels, selectedNote->harmonicType.empty() ? "none" : selectedNote->harmonicType);
            ImGui::SetNextItemWidth(198.0f);
            if (ImGui::Combo("##bottom_note_harmonic", &harmonicTypeIndex, track_editor::kHarmonicTypeLabels.data(), static_cast<int>(track_editor::kHarmonicTypeLabels.size())))
            {
                track_editor::assignComboValue(selectedNote->harmonicType, track_editor::kHarmonicTypeLabels, harmonicTypeIndex);
            }
            ImGui::SameLine();
            int pluckStyleIndex = track_editor::comboIndexFromValue(track_editor::kPluckStyleLabels, selectedNote->pluckStyle.empty() ? "none" : selectedNote->pluckStyle);
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::Combo("##bottom_note_pluck", &pluckStyleIndex, track_editor::kPluckStyleLabels.data(), static_cast<int>(track_editor::kPluckStyleLabels.size())))
            {
                track_editor::assignComboValue(selectedNote->pluckStyle, track_editor::kPluckStyleLabels, pluckStyleIndex);
            }

            ImGui::SetCursorPos(ImVec2(14.0f, chipRow1Y));
            track_editor::drawToggleChip("Hammer", selectedNote->hammerOn);
            ImGui::SameLine();
            track_editor::drawToggleChip("Pull", selectedNote->pullOff);
            ImGui::SameLine();
            track_editor::drawToggleChip("Bend", selectedNote->bend);
            ImGui::SameLine();
            track_editor::drawToggleChip("Vibrato", selectedNote->vibrato);
            ImGui::SameLine();
            track_editor::drawToggleChip("Palm", selectedNote->palmMute);
            ImGui::SameLine();
            track_editor::drawToggleChip("Ring", selectedNote->letRing);

            ImGui::SetCursorPos(ImVec2(14.0f, chipRow2Y));
            track_editor::drawToggleChip("Stacc", selectedNote->staccato);
            ImGui::SameLine();
            track_editor::drawToggleChip("Trem", selectedNote->tremoloPicking);
            ImGui::SameLine();
            track_editor::drawToggleChip("Trill", selectedNote->trill);
            ImGui::SameLine();
            track_editor::drawToggleChip("Accent", selectedNote->accent);
            ImGui::SameLine();
            track_editor::drawToggleChip("Heavy", selectedNote->heavyAccent);
        }

        if (!notesEqual(chart_.notes(), inspectorFrameBeforeNotes) && !noteInspectorEditActive_)
        {
            noteInspectorBeforeNotes_ = inspectorFrameBeforeNotes;
            noteInspectorBeforeSelection_ = inspectorFrameBeforeSelection;
            noteInspectorEditActive_ = true;
        }
        if (noteInspectorEditActive_ && !ImGui::IsAnyItemActive())
        {
            if (!notesEqual(chart_.notes(), noteInspectorBeforeNotes_))
            {
                pushNoteEditCommand("Edit note", noteInspectorBeforeNotes_, noteInspectorBeforeSelection_);
                statusMessage_ = "Edited note.";
            }
            noteInspectorEditActive_ = false;
            noteInspectorBeforeNotes_.clear();
            noteInspectorBeforeSelection_.clear();
        }
    }
    else
    {
        if (noteInspectorEditActive_ && !ImGui::IsAnyItemActive())
        {
            if (!notesEqual(chart_.notes(), noteInspectorBeforeNotes_))
            {
                pushNoteEditCommand("Edit note", noteInspectorBeforeNotes_, noteInspectorBeforeSelection_);
                statusMessage_ = "Edited note.";
            }
            noteInspectorEditActive_ = false;
            noteInspectorBeforeNotes_.clear();
            noteInspectorBeforeSelection_.clear();
        }
        const std::vector<std::string> stringLabels = currentStringLabels();
        ImGui::SetCursorPos(ImVec2(14.0f, 16.0f));
        ImGui::TextColored(track_editor::kAccent, "%s", currentPartName().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("Select a note to edit techniques. Current tuning:");
        ImGui::SetCursorPos(ImVec2(14.0f, 48.0f));
        for (size_t i = 0; i < stringLabels.size(); ++i)
        {
            if (i > 0)
            {
                ImGui::SameLine();
            }
            ImGui::TextDisabled("%d:%s", static_cast<int>(i + 1), stringLabels[i].c_str());
        }
    }

    ImGui::SetCursorPos(ImVec2(14.0f, transportControlsY));
    if (ui_.button(previewPlayer_->isPlaying() ? "Pause" : "Play", ImVec2(84.0f, 30.0f)))
    {
        togglePreviewPlayback();
    }
    ImGui::SameLine();
    if (ui_.button("Stop", ImVec2(84.0f, 30.0f)))
    {
        stopPreviewToStart();
    }
    ImGui::SameLine();
    if (ui_.button("Set Preview Here", ImVec2(134.0f, 30.0f)))
    {
        chart_.setPreviewStartSeconds(clampTransportCursor(transportCursorSeconds_));
        statusMessage_ = "Preview timestamp updated.";
    }
    ImGui::SameLine();
    const std::string stampText = track_editor::formatClock(cursorSeconds) + " / " + track_editor::formatClock(songDurationSeconds());
    const ImVec2 stampPos = ImGui::GetCursorScreenPos();
    dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.7f, stampPos, ImGui::GetColorU32(track_editor::kAccent), stampText.c_str());
    const ImVec2 stampSize = ImGui::CalcTextSize(stampText.c_str());
    ImGui::Dummy(ImVec2(stampSize.x * 1.7f, ImGui::GetFontSize() * 1.8f));

    ImGui::SameLine();
    bool timingSettingsChanged = false;
    ImGui::SetNextItemWidth(92.0f);
    timingSettingsChanged |= ImGui::SliderFloat("Song", &songVolume_, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    timingSettingsChanged |= ImGui::Checkbox("Mute##song", &songMuted_);
    ImGui::SameLine();
    timingSettingsChanged |= ImGui::Checkbox("Metro", &metronomeEnabled_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(82.0f);
    timingSettingsChanged |= ImGui::SliderFloat("##metro_vol", &metronomeVolume_, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    timingSettingsChanged |= ImGui::Checkbox("Mute##metro", &metronomeMuted_);
    ImGui::SameLine();
    timingSettingsChanged |= ImGui::Checkbox("Preview chart notes", &noteClicksEnabled_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(82.0f);
    timingSettingsChanged |= ImGui::SliderFloat("##note_vol", &notePreviewVolume_, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    timingSettingsChanged |= ImGui::Checkbox("Mute##notes", &notePreviewMuted_);
    if (timingSettingsChanged)
    {
        refreshTimingPreview();
    }

    if (showTimingDiagnostics_)
    {
        if (ImGui::Begin("Timing Diagnostics", &showTimingDiagnostics_, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const openchordix::track::TempoMap map = currentTempoMap();
            const double chartSeconds =
                openchordix::track::editor::chartSecondsFromAudioSeconds(cursorSeconds, chart_.chartAudioOffsetMs());
            const int cursorTick = static_cast<int>(std::lround(timelineTickFromSeconds(cursorSeconds)));
            const auto nextTempo = map.nextEventAfterTick(cursorTick);
            const auto firstNote = std::min_element(chart_.notes().begin(), chart_.notes().end(),
                                                    [](const TrackTabNote &left, const TrackTabNote &right)
                                                    { return left.tick < right.tick; });
            ImGui::Text("Ticks per beat: %d", map.ticksPerBeat());
            ImGui::Text("Audio time: %.3f", cursorSeconds);
            ImGui::Text("Chart offset: %d ms", chart_.chartAudioOffsetMs());
            ImGui::Text("Adjusted chart time: %.3f", chartSeconds);
            ImGui::Text("Chart tick: %d", cursorTick);
            ImGui::Text("Current BPM: %.3f", map.bpmAtTick(cursorTick));
            ImGui::Text("Next tempo: %s", nextTempo ? ("tick " + std::to_string(nextTempo->tick) + " @ " + std::to_string(nextTempo->bpm)).c_str() : "none");
            ImGui::Text("Tempo events: %zu", map.events().size());
            ImGui::TextWrapped("Offset fixes constant start mismatch only. If sync gets worse over time, audit imported tempo events and tempo-map seconds.");
            if (firstNote != chart_.notes().end())
            {
                ImGui::Text("First note: tick %d | chart %.3fs | audio %.3fs",
                            firstNote->tick,
                            map.tickToSeconds(firstNote->tick),
                            openchordix::track::editor::audioSecondsFromChartSeconds(
                                map.tickToSeconds(firstNote->tick), chart_.chartAudioOffsetMs()));
            }
            if (ImGui::BeginTable("tempo_events_debug", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("Tick");
                ImGui::TableSetupColumn("BPM");
                ImGui::TableSetupColumn("Seconds");
                ImGui::TableHeadersRow();
                const auto &events = map.events();
                for (std::size_t index = 0; index < std::min<std::size_t>(events.size(), 20); ++index)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", events[index].tick);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", events[index].bpm);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.3f", map.tickToSeconds(events[index].tick));
                }
                ImGui::EndTable();
            }
            if (selectedNote != nullptr)
            {
                ImGui::Separator();
                ImGui::Text("Selected note");
                ImGui::Text("Tick: %d", selectedNote->tick);
                ImGui::Text("Duration ticks: %d", selectedNote->duration);
                ImGui::Text("Start seconds: chart %.3f | audio %.3f",
                            map.tickToSeconds(selectedNote->tick),
                            openchordix::track::editor::audioSecondsFromChartSeconds(
                                map.tickToSeconds(selectedNote->tick), chart_.chartAudioOffsetMs()));
                ImGui::Text("End seconds: chart %.3f | audio %.3f",
                            map.tickToSeconds(selectedNote->tick + selectedNote->duration),
                            openchordix::track::editor::audioSecondsFromChartSeconds(
                                map.tickToSeconds(selectedNote->tick + selectedNote->duration),
                                chart_.chartAudioOffsetMs()));
                ImGui::Text("String: %d", selectedNote->stringIndex + 1);
                ImGui::Text("Fret: %d", selectedNote->fret);
            }
        }
        ImGui::End();
    }

    const ImVec2 stripPos(min.x + 14.0f, min.y + transportStripY);
    const ImVec2 stripSize((screen.x - 24.0f) - 28.0f, transportStripHeight);
    const ImVec2 stripMin = stripPos;
    const ImVec2 stripMax(stripPos.x + stripSize.x, stripPos.y + stripSize.y);
    dl->AddRectFilled(stripMin, stripMax, ImGui::GetColorU32(ImVec4(0.06f, 0.08f, 0.12f, 1.0f)), 10.0f);
    dl->AddRect(stripMin, stripMax, ImGui::GetColorU32(track_editor::kBorder), 10.0f, 0, 1.0f);

    const int totalTicks = timelineTotalTicks();
    const auto xFromSeconds = [&](double seconds)
    {
        const double tick = std::clamp(timelineTickFromSeconds(seconds), 0.0, static_cast<double>(totalTicks));
        return stripMin.x + static_cast<float>(tick / static_cast<double>(totalTicks) * stripSize.x);
    };

    const int majorTicks = std::max(8, std::min(songDurationSeconds() / 15, 48));
    for (int i = 0; i <= majorTicks; ++i)
    {
        const float x = stripMin.x + stripSize.x * (static_cast<float>(i) / static_cast<float>(majorTicks));
        const float y1 = stripMin.y + ((i % 4 == 0) ? 4.0f : 10.0f);
        dl->AddLine(ImVec2(x, y1), ImVec2(x, stripMax.y - 4.0f), ImGui::GetColorU32(i % 4 == 0 ? track_editor::kMeasure : track_editor::kGrid));
    }

    const float previewX = xFromSeconds(chart_.previewStartSeconds());
    const float playX = xFromSeconds(cursorSeconds);
    dl->AddLine(ImVec2(previewX, stripMin.y + 2.0f), ImVec2(previewX, stripMax.y - 2.0f), ImGui::GetColorU32(track_editor::kPreviewMarker), 2.0f);
    dl->AddLine(ImVec2(playX, stripMin.y + 1.0f), ImVec2(playX, stripMax.y - 1.0f), ImGui::GetColorU32(track_editor::kPlayhead), 2.0f);

    ImGui::SetCursorScreenPos(stripMin);
    ImGui::InvisibleButton("transport_strip", stripSize);
    const bool stripHovered = ImGui::IsItemHovered();
    if (stripHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        scrubbingTransport_ = true;
        scrubResumePlayback_ = previewPlayer_->isPlaying();
        if (scrubResumePlayback_)
        {
            pausePreviewPlayback();
        }
    }
    if (scrubbingTransport_)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            const float relative = std::clamp((ImGui::GetIO().MousePos.x - stripMin.x) / stripSize.x, 0.0f, 1.0f);
            const double tick = static_cast<double>(totalTicks) * static_cast<double>(relative);
            transportCursorSeconds_ = clampTransportCursor(timelineSecondsFromTick(tick));
            syncTimelineScrollToSeconds(transportCursorSeconds_);
        }
        else
        {
            scrubbingTransport_ = false;
            if (scrubResumePlayback_)
            {
                startPreviewFromCursor();
            }
            scrubResumePlayback_ = false;
        }
    }

    ImGui::EndChild();
}
