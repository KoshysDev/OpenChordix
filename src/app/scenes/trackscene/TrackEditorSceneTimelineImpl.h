#pragma once

#include "track/TrackTiming.h"

void TrackEditorScene::drawTimeline(const ImVec2 &screen, float top, float dt)
{
    ImGui::SetCursorPos(ImVec2(12.0f, top));
    ImGui::BeginChild("track_editor_timeline_host",
                      ImVec2(screen.x - 24.0f, screen.y - top - track_editor::kBottomBarHeight - 12.0f),
                      false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImVec2 hostMin = ImGui::GetWindowPos();
    const ImVec2 hostMax(hostMin.x + screen.x - 24.0f, hostMin.y + screen.y - top - track_editor::kBottomBarHeight - 12.0f);
    track_editor::drawPanelFrame(dl, hostMin, hostMax, track_editor::kPanelSoft);

    ensureNoteEditorIds();
    const std::string activePart = currentPartName();
    const std::vector<std::string> stringLabels = currentStringLabels();
    const int stringCount = static_cast<int>(stringLabels.size());
    const int totalBeats = timelineTotalBeats();
    const int totalTicks = timelineTotalTicks();

    ImGui::SetCursorPos(ImVec2(12.0f, 12.0f));
    const ImVec2 viewSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("timeline_canvas", viewSize);
    const bool timelineHovered = ImGui::IsItemHovered();
    const ImVec2 canvasMin = ImGui::GetItemRectMin();
    const ImVec2 canvasMax = ImGui::GetItemRectMax();
    const float canvasHeight = canvasMax.y - canvasMin.y;
    const float viewportWidth = canvasMax.x - canvasMin.x;
    dl = ImGui::GetWindowDrawList();
    const float rowHeight = std::clamp(
        (canvasHeight - track_editor::kRulerHeight - 18.0f) /
            static_cast<float>(std::clamp(std::max(stringCount + 2, 8), stringCount, openchordix::track::kMaxTrackStrings)),
        38.0f,
        68.0f);

    float pixelsPerBeat = 94.0f * zoom_;
    float pixelsPerTick = pixelsPerBeat / static_cast<float>(chart_.ticksPerBeat());
    float canvasWidth = track_editor::kLaneLabelWidth + totalTicks * pixelsPerTick + 180.0f;
    float maxScroll = std::max(0.0f, canvasWidth - viewportWidth);
    timelineViewportWidth_ = viewportWidth;
    if (timelineSyncPending_)
    {
        syncTimelineScrollToSeconds(transportCursorSeconds_, viewportWidth);
    }
    timelineScrollX_ = std::clamp(timelineScrollX_, 0.0f, maxScroll);

    const bool previewPlaying = previewPlayer_->isPlaying();
    const ImGuiIO &io = ImGui::GetIO();
    const ImVec2 mousePos = io.MousePos;
    const bool dragAutoScrollActive = (selectionDragActive_ || draggingNote_) && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (dragAutoScrollActive)
    {
        const float velocity = openchordix::track::editor::computeTimelineAutoScrollVelocity(
            mousePos.x,
            openchordix::track::editor::EditorRect{canvasMin.x, canvasMin.y, canvasMax.x, canvasMax.y});
        timelineScrollX_ = openchordix::track::editor::applyTimelineAutoScroll(
            timelineScrollX_, velocity, dt, maxScroll);
    }
    if (!previewPlaying && timelineHovered && !ImGui::IsAnyItemActive())
    {
        if (io.KeyCtrl && std::fabs(io.MouseWheel) > 0.0f)
        {
            const float localX = std::max(0.0f, mousePos.x - canvasMin.x - track_editor::kLaneLabelWidth);
            const float anchorTick = (timelineScrollX_ + localX) / std::max(1.0f, pixelsPerTick);
            const float zoomStep = io.MouseWheel > 0.0f ? 1.12f : (1.0f / 1.12f);
            zoom_ = std::clamp(zoom_ * zoomStep, track_editor::kMinZoom, track_editor::kMaxZoom);
            pixelsPerBeat = 94.0f * zoom_;
            pixelsPerTick = pixelsPerBeat / static_cast<float>(chart_.ticksPerBeat());
            canvasWidth = track_editor::kLaneLabelWidth + totalTicks * pixelsPerTick + 180.0f;
            maxScroll = std::max(0.0f, canvasWidth - viewportWidth);
            timelineScrollX_ = std::clamp(anchorTick * pixelsPerTick - localX, 0.0f, maxScroll);
        }
        else if (std::fabs(io.MouseWheel) > 0.0f || std::fabs(io.MouseWheelH) > 0.0f)
        {
            timelineScrollX_ = std::clamp(timelineScrollX_ - io.MouseWheel * 160.0f - io.MouseWheelH * 160.0f, 0.0f, maxScroll);
        }
    }

    const double cursorSeconds = displayedCursorSeconds();
    if (previewPlaying && !scrubbingTransport_ && !dragAutoScrollActive)
    {
        syncTimelineScrollToSeconds(cursorSeconds, viewportWidth);
        timelineScrollX_ = std::clamp(timelineScrollX_, 0.0f, maxScroll);
    }

    dl->AddRectFilled(canvasMin, canvasMax, ImGui::GetColorU32(ImVec4(0.05f, 0.07f, 0.10f, 1.0f)));
    dl->AddRect(canvasMin, canvasMax, ImGui::GetColorU32(track_editor::kBorder));
    dl->PushClipRect(canvasMin, canvasMax, true);

    for (int stringIndex = 0; stringIndex < stringCount; ++stringIndex)
    {
        const float y = canvasMin.y + track_editor::kRulerHeight + rowHeight * stringIndex;
        dl->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), ImGui::GetColorU32(track_editor::kGrid));
        const std::string &label = stringLabels[static_cast<size_t>(stringIndex)];
        dl->AddText(ImVec2(canvasMin.x + 12.0f, y + rowHeight * 0.5f - 8.0f), ImGui::GetColorU32(track_editor::kMuted), label.c_str());
    }

    const auto drawGridLine = [&](int tick, bool isMeasure, const std::string &label)
    {
        const float x = canvasMin.x + track_editor::kLaneLabelWidth + tick * pixelsPerTick - timelineScrollX_;
        if (x < canvasMin.x + track_editor::kLaneLabelWidth - pixelsPerBeat || x > canvasMax.x + pixelsPerBeat)
        {
            return;
        }
        dl->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, canvasMin.y + track_editor::kRulerHeight + rowHeight * stringCount),
                    ImGui::GetColorU32(isMeasure ? track_editor::kMeasure : track_editor::kGrid),
                    isMeasure ? 2.0f : 1.0f);
        if (!label.empty())
        {
            dl->AddText(ImVec2(x + 4.0f, canvasMin.y + 8.0f), ImGui::GetColorU32(track_editor::kMuted), label.c_str());
        }
    };

    if (chart_.measures().empty())
    {
        for (int beat = 0; beat <= totalBeats; ++beat)
        {
            const bool isMeasure = beat % chart_.beatsPerMeasure() == 0;
            drawGridLine(beat * chart_.ticksPerBeat(), isMeasure,
                         beat < totalBeats ? std::to_string(beat + 1) : "");
        }
    }
    else
    {
        for (const TrackChartMeasure &measure : chart_.measures())
        {
            for (int beat = 0; beat < measure.numerator; ++beat)
            {
                const int tick = measure.startTick +
                                 openchordix::track::beatOffsetTicksInMeasure(
                                     chart_.ticksPerBeat(), beat, measure.denominator);
                const std::string label = beat == 0
                                              ? "M" + std::to_string(measure.number)
                                              : std::to_string(measure.number) + "." + std::to_string(beat + 1);
                drawGridLine(tick, beat == 0, label);
            }
        }
        const TrackChartMeasure &last = chart_.measures().back();
        drawGridLine(last.startTick + last.durationTicks, true, "");
    }

    const float previewTick = static_cast<float>(timelineTickFromSeconds(chart_.previewStartSeconds()));
    const float previewX = canvasMin.x + track_editor::kLaneLabelWidth + previewTick * pixelsPerTick - timelineScrollX_;
    dl->AddLine(ImVec2(previewX, canvasMin.y), ImVec2(previewX, canvasMin.y + canvasHeight), ImGui::GetColorU32(track_editor::kPreviewMarker), 2.0f);

    const float transportTick = static_cast<float>(timelineTickFromSeconds(cursorSeconds));
    const float playheadX = canvasMin.x + track_editor::kLaneLabelWidth + transportTick * pixelsPerTick - timelineScrollX_;
    dl->AddLine(ImVec2(playheadX, canvasMin.y), ImVec2(playheadX, canvasMin.y + canvasHeight), ImGui::GetColorU32(track_editor::kPlayhead), 2.0f);

    const auto rawTickAt = [&](float x)
    {
        return openchordix::track::editor::screenXToTimelineTick(
            x, canvasMin.x, timelineScrollX_, track_editor::kLaneLabelWidth, pixelsPerTick);
    };
    const auto stringAt = [&](float y)
    {
        return openchordix::track::editor::screenYToTimelineLane(
            y, canvasMin.y, track_editor::kRulerHeight, rowHeight, stringCount);
    };
    const auto selectionModeFromInput = [&]()
    {
        if (io.KeyCtrl)
        {
            return openchordix::track::editor::SelectionMode::Toggle;
        }
        if (io.KeyShift)
        {
            return openchordix::track::editor::SelectionMode::Add;
        }
        return openchordix::track::editor::SelectionMode::Replace;
    };
    const auto previewTickForNote = [&](const TrackTabNote &note)
    {
        if (!draggingNote_ || !isNoteSelected(note.editorId))
        {
            return note.tick;
        }
        for (const auto &original : draggedSelectionOriginalNotes_)
        {
            if (original.id == note.editorId)
            {
                return openchordix::track::editor::movedTickForDraggedNote(original, currentDragDeltaTicks_);
            }
        }
        return note.tick;
    };
    const auto noteBadgeText = [](const TrackTabNote &note)
    {
        std::string badge;
        if (note.noteType != "normal" && !note.noteType.empty())
        {
            badge += note.noteType.substr(0, 1);
        }
        if (note.hammerOn)
        {
            badge += badge.empty() ? "HO" : " HO";
        }
        if (note.pullOff)
        {
            badge += badge.empty() ? "PO" : " PO";
        }
        if (!note.slideType.empty())
        {
            badge += badge.empty() ? "SL" : " SL";
        }
        if (note.bend)
        {
            badge += badge.empty() ? "B" : " B";
        }
        if (note.vibrato)
        {
            badge += badge.empty() ? "V" : " V";
        }
        if (badge.size() > 10)
        {
            badge.resize(10);
        }
        return badge;
    };

    if (selectionDragActive_)
    {
        selectionCurrentMouse_ = mousePos;
        selectionCurrentTick_ = std::max(0, rawTickAt(mousePos.x));
        selectionCurrentLane_ = stringAt(mousePos.y);
        const float dx = selectionCurrentMouse_.x - selectionStartMouse_.x;
        const float dy = selectionCurrentMouse_.y - selectionStartMouse_.y;
        selectionDragExceededThreshold_ = selectionDragExceededThreshold_ || (dx * dx + dy * dy) >= 16.0f;
        if (selectionDragExceededThreshold_)
        {
            const auto selectionRange = openchordix::track::editor::normalizedSelectionRange(
                selectionStartTick_, selectionCurrentTick_, selectionStartLane_, selectionCurrentLane_);
            const float startX = openchordix::track::editor::timelineTickToScreenX(
                selectionRange.minTick, canvasMin.x, timelineScrollX_, track_editor::kLaneLabelWidth, pixelsPerTick);
            const float endX = openchordix::track::editor::timelineTickToScreenX(
                selectionRange.maxTick, canvasMin.x, timelineScrollX_, track_editor::kLaneLabelWidth, pixelsPerTick);
            const float startY = canvasMin.y + track_editor::kRulerHeight + rowHeight * selectionRange.minLane;
            const float endY = canvasMin.y + track_editor::kRulerHeight + rowHeight * (selectionRange.maxLane + 1);
            const auto selectionRect = openchordix::track::editor::normalizedRect(startX, startY, endX, endY);
            dl->AddRectFilled(ImVec2(selectionRect.minX, selectionRect.minY),
                              ImVec2(selectionRect.maxX, selectionRect.maxY),
                              ImGui::GetColorU32(ImVec4(0.34f, 0.78f, 0.98f, 0.18f)));
            dl->AddRect(ImVec2(selectionRect.minX, selectionRect.minY),
                        ImVec2(selectionRect.maxX, selectionRect.maxY),
                        ImGui::GetColorU32(ImVec4(0.60f, 0.90f, 1.0f, 0.88f)),
                        3.0f,
                        0,
                        1.4f);
        }
    }

    int hoveredNoteIndex = -1;
    openchordix::track::editor::EditorNoteId hoveredNoteId = 0;
    for (int i = 0; i < static_cast<int>(chart_.notes().size()); ++i)
    {
        const TrackTabNote &note = chart_.notes()[i];
        if (note.part != activePart)
        {
            continue;
        }

        const int displayTick = previewTickForNote(note);
        const auto noteRect = openchordix::track::editor::noteVisualRect(
            displayTick,
            note.duration,
            note.stringIndex,
            canvasMin.x,
            canvasMin.y,
            timelineScrollX_,
            track_editor::kLaneLabelWidth,
            pixelsPerTick,
            track_editor::kRulerHeight,
            rowHeight);
        const ImVec2 noteMin(noteRect.minX, noteRect.minY);
        const ImVec2 noteMax(noteRect.maxX, noteRect.maxY);
        if (noteMax.x < canvasMin.x + track_editor::kLaneLabelWidth || noteMin.x > canvasMax.x)
        {
            continue;
        }
        const bool hovered = mousePos.x >= noteMin.x && mousePos.x <= noteMax.x && mousePos.y >= noteMin.y && mousePos.y <= noteMax.y;
        const bool selected = isNoteSelected(note.editorId);
        dl->AddRectFilled(noteMin, noteMax,
                          ImGui::GetColorU32(selected ? ImVec4(0.42f, 0.86f, 1.0f, 1.0f)
                                                      : hovered ? ImVec4(0.30f, 0.66f, 0.94f, 0.96f)
                                                                : track_editor::kAccentSoft),
                          8.0f);
        dl->AddRect(noteMin, noteMax,
                    ImGui::GetColorU32(selected ? ImVec4(0.96f, 0.98f, 1.0f, 1.0f) : ImVec4(0.12f, 0.18f, 0.24f, 1.0f)),
                    8.0f,
                    0,
                    1.2f);
        const std::string fretText = std::to_string(note.fret);
        dl->AddText(ImVec2(noteMin.x + 8.0f, noteMin.y + 8.0f), ImGui::GetColorU32(ImVec4(0.98f, 0.99f, 1.0f, 1.0f)), fretText.c_str());
        const std::string badge = noteBadgeText(note);
        if (!badge.empty())
        {
            dl->AddText(ImVec2(noteMin.x + 8.0f, noteMin.y + 26.0f), ImGui::GetColorU32(ImVec4(0.83f, 0.93f, 1.0f, 0.95f)), badge.c_str());
        }

        if (hovered)
        {
            hoveredNoteIndex = i;
            hoveredNoteId = note.editorId;
        }
    }

    if (draggingNote_)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && draggingNoteId_ != 0)
        {
            const int rawDelta = rawTickAt(mousePos.x) - dragStartMouseTick_;
            const int snappedDelta = openchordix::track::editor::snapDeltaTicks(rawDelta, snapTickSize());
            currentDragDeltaTicks_ =
                openchordix::track::editor::clampGroupDragDeltaAtZero(draggedSelectionOriginalNotes_, snappedDelta);
        }
        else
        {
            const int committedDelta = currentDragDeltaTicks_;
            draggingNote_ = false;
            draggingNoteId_ = 0;
            const int movedCount = commitDraggedSelectionMove(committedDelta);
            if (movedCount > 0)
            {
                statusMessage_ = "Moved " + std::to_string(movedCount) +
                                 (movedCount == 1 ? " note" : " notes") +
                                 " by " + std::to_string(committedDelta) + " ticks.";
            }
            draggedSelectionOriginalNotes_.clear();
            currentDragDeltaTicks_ = 0;
        }
    }

    if (selectionDragActive_)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (selectionDragExceededThreshold_)
            {
                const auto selectionRange = openchordix::track::editor::normalizedSelectionRange(
                    selectionStartTick_, selectionCurrentTick_, selectionStartLane_, selectionCurrentLane_);
                std::vector<openchordix::track::editor::EditorNoteId> hits;
                for (const TrackTabNote &note : chart_.notes())
                {
                    if (note.part == activePart &&
                        openchordix::track::editor::noteIntersectsSelectionRange(
                            note.tick, note.duration, note.stringIndex, selectionRange))
                    {
                        hits.push_back(note.editorId);
                    }
                }
                selectedNoteIds_ = openchordix::track::editor::applySelection(selectedNoteIds_, hits, selectionMode_);
                syncSelectedNoteIndex();
                statusMessage_ = "Selected " + std::to_string(selectedNoteIds_.size()) +
                                 (selectedNoteIds_.size() == 1 ? " note." : " notes.");
            }
            else if (editorTool_ == EditorTool::Draw &&
                     selectionStartMouse_.y <= canvasMin.y + track_editor::kRulerHeight + rowHeight * stringCount)
            {
                TrackTabNote note;
                const std::vector<TrackTabNote> beforeNotes = chart_.notes();
                const std::vector<openchordix::track::editor::EditorNoteId> beforeSelection = selectedNoteIds_;
                note.editorId = nextEditorNoteId_++;
                note.part = activePart;
                note.tick = quantizeTick(std::clamp(rawTickAt(selectionStartMouse_.x), 0, totalTicks - 1));
                note.duration = snapTickSize();
                note.stringIndex = stringAt(selectionStartMouse_.y);
                note.fret = 0;
                const auto newId = note.editorId;
                chart_.notes().push_back(note);
                sortNotes();
                setPrimarySelectedNote(newId);
                pushNoteEditCommand("Add note", beforeNotes, beforeSelection);
            }
            else if (selectionMode_ == openchordix::track::editor::SelectionMode::Replace)
            {
                clearNoteSelection();
            }
            selectionDragActive_ = false;
            selectionDragExceededThreshold_ = false;
        }
    }

    if (timelineHovered && !selectionDragActive_)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            if (hoveredNoteIndex >= 0)
            {
                setPrimarySelectedNote(chart_.notes()[static_cast<size_t>(hoveredNoteIndex)].editorId);
                deleteSelectedNotes();
                draggingNote_ = false;
                draggingNoteId_ = 0;
                draggedSelectionOriginalNotes_.clear();
            }
            else if (mousePos.y <= canvasMin.y + track_editor::kRulerHeight)
            {
                chart_.setPreviewStartSeconds(clampTransportCursor(timelineSecondsFromTick(std::max(0, rawTickAt(mousePos.x)))));
                statusMessage_ = "Preview timestamp updated.";
            }
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if (mousePos.y <= canvasMin.y + track_editor::kRulerHeight)
            {
                transportCursorSeconds_ = clampTransportCursor(timelineSecondsFromTick(std::max(0, rawTickAt(mousePos.x))));
            }
            else if (hoveredNoteIndex >= 0)
            {
                if (editorTool_ == EditorTool::Erase)
                {
                    setPrimarySelectedNote(chart_.notes()[static_cast<size_t>(hoveredNoteIndex)].editorId);
                    deleteSelectedNotes();
                }
                else
                {
                    if (io.KeyCtrl)
                    {
                        selectedNoteIds_ = openchordix::track::editor::applySelection(
                            selectedNoteIds_, {hoveredNoteId}, openchordix::track::editor::SelectionMode::Toggle);
                    }
                    else if (io.KeyShift)
                    {
                        selectedNoteIds_ = openchordix::track::editor::applySelection(
                            selectedNoteIds_, {hoveredNoteId}, openchordix::track::editor::SelectionMode::Add);
                    }
                    else if (!isNoteSelected(hoveredNoteId))
                    {
                        setPrimarySelectedNote(hoveredNoteId);
                    }
                    syncSelectedNoteIndex();

                    if (isNoteSelected(hoveredNoteId))
                    {
                        draggingNote_ = true;
                        draggingNoteId_ = hoveredNoteId;
                        dragStartMouseTick_ = rawTickAt(mousePos.x);
                        currentDragDeltaTicks_ = 0;
                        draggedSelectionOriginalNotes_.clear();
                        draggedSelectionOriginalNotes_.reserve(selectedNoteIds_.size());
                        for (const TrackTabNote &note : chart_.notes())
                        {
                            if (note.part == activePart && isNoteSelected(note.editorId))
                            {
                                draggedSelectionOriginalNotes_.push_back({
                                    note.editorId,
                                    note.tick,
                                    note.stringIndex,
                                    note.fret,
                                });
                            }
                        }
                    }
                }
            }
            else if (mousePos.y <= canvasMin.y + track_editor::kRulerHeight + rowHeight * stringCount)
            {
                selectionDragActive_ = true;
                selectionDragExceededThreshold_ = false;
                selectionStartMouse_ = mousePos;
                selectionCurrentMouse_ = mousePos;
                selectionStartTick_ = std::max(0, rawTickAt(mousePos.x));
                selectionCurrentTick_ = selectionStartTick_;
                selectionStartLane_ = stringAt(mousePos.y);
                selectionCurrentLane_ = selectionStartLane_;
                selectionMode_ = selectionModeFromInput();
            }
        }
    }

    if (draggingNote_ && currentDragDeltaTicks_ != 0)
    {
        const std::string deltaText = "Move: " + std::string(currentDragDeltaTicks_ > 0 ? "+" : "") +
                                      std::to_string(currentDragDeltaTicks_) + " ticks";
        dl->AddText(ImVec2(canvasMin.x + track_editor::kLaneLabelWidth + 12.0f, canvasMin.y + 10.0f),
                    ImGui::GetColorU32(ImVec4(0.88f, 0.96f, 1.0f, 0.95f)),
                    deltaText.c_str());
    }

    dl->PopClipRect();
    timelineViewportWidth_ = viewportWidth;
    ImGui::EndChild();
}
