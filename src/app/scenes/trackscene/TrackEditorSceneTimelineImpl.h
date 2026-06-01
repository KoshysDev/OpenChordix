#pragma once

#include "track/TrackTiming.h"

void TrackEditorScene::drawTimeline(const ImVec2 &screen, float top)
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
    if (previewPlaying && !scrubbingTransport_)
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
        return static_cast<int>(std::floor((x - canvasMin.x + timelineScrollX_ - track_editor::kLaneLabelWidth) / pixelsPerTick));
    };
    const auto stringAt = [&](float y)
    {
        return std::clamp(static_cast<int>((y - canvasMin.y - track_editor::kRulerHeight) / rowHeight), 0, stringCount - 1);
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

    int hoveredNoteIndex = -1;
    for (int i = 0; i < static_cast<int>(chart_.notes().size()); ++i)
    {
        const TrackTabNote &note = chart_.notes()[i];
        if (note.part != activePart)
        {
            continue;
        }

        const float x = canvasMin.x + track_editor::kLaneLabelWidth + note.tick * pixelsPerTick - timelineScrollX_;
        const float noteVerticalPadding = std::max(8.0f, rowHeight * 0.12f);
        const float y = canvasMin.y + track_editor::kRulerHeight + rowHeight * note.stringIndex + noteVerticalPadding;
        const float widthPx = std::max(32.0f, note.duration * pixelsPerTick - 6.0f);
        const ImVec2 noteMin(x, y);
        const ImVec2 noteMax(x + widthPx, y + rowHeight - noteVerticalPadding * 2.0f);
        if (noteMax.x < canvasMin.x + track_editor::kLaneLabelWidth || noteMin.x > canvasMax.x)
        {
            continue;
        }
        const bool hovered = mousePos.x >= noteMin.x && mousePos.x <= noteMax.x && mousePos.y >= noteMin.y && mousePos.y <= noteMax.y;
        const bool selected = selectedNoteIndex_ == i;

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
        }
    }

    if (draggingNote_)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && draggingNoteIndex_ >= 0 &&
            draggingNoteIndex_ < static_cast<int>(chart_.notes().size()))
        {
            TrackTabNote &note = chart_.notes()[draggingNoteIndex_];
            note.tick = quantizeTick(std::clamp(rawTickAt(mousePos.x) - dragGrabTickOffset_, 0, totalTicks - 1));
            note.stringIndex = std::clamp(stringAt(mousePos.y) - dragGrabStringOffset_, 0, stringCount - 1);
        }
        else
        {
            const bool validDraggedIndex = draggingNoteIndex_ >= 0 && draggingNoteIndex_ < static_cast<int>(chart_.notes().size());
            const TrackTabNote dragged = validDraggedIndex ? chart_.notes()[draggingNoteIndex_] : TrackTabNote{};
            draggingNote_ = false;
            draggingNoteIndex_ = -1;
            if (validDraggedIndex)
            {
                sortNotes();
                selectedNoteIndex_ = -1;
                for (int i = 0; i < static_cast<int>(chart_.notes().size()); ++i)
                {
                    const TrackTabNote &note = chart_.notes()[i];
                    if (note.part == dragged.part && note.tick == dragged.tick && note.duration == dragged.duration &&
                        note.stringIndex == dragged.stringIndex && note.fret == dragged.fret)
                    {
                        selectedNoteIndex_ = i;
                        break;
                    }
                }
            }
            else
            {
                selectedNoteIndex_ = -1;
            }
        }
    }

    if (timelineHovered)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            if (hoveredNoteIndex >= 0)
            {
                chart_.notes().erase(chart_.notes().begin() + hoveredNoteIndex);
                selectedNoteIndex_ = -1;
                draggingNote_ = false;
                draggingNoteIndex_ = -1;
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
                    chart_.notes().erase(chart_.notes().begin() + hoveredNoteIndex);
                    selectedNoteIndex_ = -1;
                }
                else
                {
                    selectedNoteIndex_ = hoveredNoteIndex;
                    if (editorTool_ == EditorTool::Move)
                    {
                        draggingNote_ = true;
                        draggingNoteIndex_ = hoveredNoteIndex;
                        dragGrabTickOffset_ = rawTickAt(mousePos.x) - chart_.notes()[hoveredNoteIndex].tick;
                        dragGrabStringOffset_ = stringAt(mousePos.y) - chart_.notes()[hoveredNoteIndex].stringIndex;
                    }
                }
            }
            else if (editorTool_ == EditorTool::Draw &&
                     mousePos.y <= canvasMin.y + track_editor::kRulerHeight + rowHeight * stringCount)
            {
                TrackTabNote note;
                note.part = activePart;
                note.tick = quantizeTick(std::clamp(rawTickAt(mousePos.x), 0, totalTicks - 1));
                note.duration = snapTickSize();
                note.stringIndex = stringAt(mousePos.y);
                note.fret = 0;
                chart_.notes().push_back(note);
                sortNotes();
                selectedNoteIndex_ = -1;
                for (int i = 0; i < static_cast<int>(chart_.notes().size()); ++i)
                {
                    const TrackTabNote &added = chart_.notes()[i];
                    if (added.part == note.part && added.tick == note.tick && added.stringIndex == note.stringIndex &&
                        added.fret == note.fret)
                    {
                        selectedNoteIndex_ = i;
                        break;
                    }
                }
            }
        }
    }

    dl->PopClipRect();
    timelineViewportWidth_ = viewportWidth;
    ImGui::EndChild();
}
