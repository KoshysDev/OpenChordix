#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openchordix::track::editor
{
    using EditorNoteId = std::uint64_t;

    inline constexpr std::array<int, 4> kSnapDivisors = {1, 2, 4, 8};

    struct EditorRect
    {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
    };

    struct EditorSelectableNoteRect
    {
        EditorNoteId id = 0;
        EditorRect rect;
    };

    struct EditorDraggedNote
    {
        EditorNoteId id = 0;
        int originalTick = 0;
        int stringIndex = 0;
        int fret = 0;
    };

    struct EditorMovedNote
    {
        EditorNoteId id = 0;
        int tick = 0;
        int durationTicks = 1;
        int stringIndex = 0;
        int fret = 0;
    };

    struct TimelineSelectionRange
    {
        int minTick = 0;
        int maxTick = 0;
        int minLane = 0;
        int maxLane = 0;
    };

    struct TimelineAutoScrollConfig
    {
        float edgeZonePx = 56.0f;
        float minPixelsPerSecond = 80.0f;
        float maxPixelsPerSecond = 900.0f;
    };

    struct EditorClipboardNote
    {
        int relativeTick = 0;
        int durationTicks = 1;
        int stringIndex = 0;
        int fret = 0;
    };

    struct EditorSnapshotCommand
    {
        std::string label;
        std::vector<EditorMovedNote> before;
        std::vector<EditorMovedNote> after;
    };

    struct EditorCommandStack
    {
        std::vector<EditorSnapshotCommand> undo;
        std::vector<EditorSnapshotCommand> redo;
    };

    struct EditorPartNoteRef
    {
        EditorNoteId id = 0;
        std::string part;
    };

    enum class SelectionMode
    {
        Replace,
        Add,
        Toggle
    };

    inline EditorRect normalizedRect(float startX, float startY, float endX, float endY)
    {
        return {
            std::min(startX, endX),
            std::min(startY, endY),
            std::max(startX, endX),
            std::max(startY, endY),
        };
    }

    inline bool rectsIntersect(const EditorRect &left, const EditorRect &right)
    {
        return left.minX <= right.maxX && left.maxX >= right.minX &&
               left.minY <= right.maxY && left.maxY >= right.minY;
    }

    inline TimelineSelectionRange normalizedSelectionRange(int startTick, int currentTick, int startLane, int currentLane)
    {
        return {
            std::min(startTick, currentTick),
            std::max(startTick, currentTick),
            std::min(startLane, currentLane),
            std::max(startLane, currentLane),
        };
    }

    inline bool noteIntersectsSelectionRange(int noteTick,
                                             int durationTicks,
                                             int noteLane,
                                             const TimelineSelectionRange &selection)
    {
        const int noteStart = std::max(0, noteTick);
        const int noteEnd = noteStart + std::max(1, durationTicks);
        return noteStart <= selection.maxTick &&
               noteEnd >= selection.minTick &&
               noteLane >= selection.minLane &&
               noteLane <= selection.maxLane;
    }

    inline float computeTimelineAutoScrollVelocity(float mouseX,
                                                   const EditorRect &timelineRect,
                                                   const TimelineAutoScrollConfig &config = {})
    {
        const float zone = std::max(1.0f, config.edgeZonePx);
        const float maxVelocity = std::max(0.0f, config.maxPixelsPerSecond);
        const float minVelocity = std::clamp(config.minPixelsPerSecond, 0.0f, maxVelocity);
        const float leftDistance = mouseX - timelineRect.minX;
        const float rightDistance = timelineRect.maxX - mouseX;

        const auto velocityForDepth = [&](float depth)
        {
            const float clampedDepth = std::clamp(depth, 0.0f, 1.0f);
            if (clampedDepth <= 0.0f || maxVelocity <= 0.0f)
            {
                return 0.0f;
            }
            return std::clamp(std::max(minVelocity, clampedDepth * maxVelocity), 0.0f, maxVelocity);
        };

        if (leftDistance < zone)
        {
            return -velocityForDepth((zone - leftDistance) / zone);
        }
        if (rightDistance < zone)
        {
            return velocityForDepth((zone - rightDistance) / zone);
        }
        return 0.0f;
    }

    inline float applyTimelineAutoScroll(float scrollX, float velocityPixelsPerSecond, float deltaSeconds, float maxScrollX)
    {
        return std::clamp(scrollX + velocityPixelsPerSecond * std::max(0.0f, deltaSeconds),
                          0.0f,
                          std::max(0.0f, maxScrollX));
    }

    inline int screenXToTimelineTick(float screenX,
                                     float canvasMinX,
                                     float timelineScrollX,
                                     float laneLabelWidth,
                                     float pixelsPerTick)
    {
        const float safePixelsPerTick = std::max(0.0001f, pixelsPerTick);
        return static_cast<int>(std::floor((screenX - canvasMinX + timelineScrollX - laneLabelWidth) / safePixelsPerTick));
    }

    inline float timelineTickToScreenX(int tick,
                                       float canvasMinX,
                                       float timelineScrollX,
                                       float laneLabelWidth,
                                       float pixelsPerTick)
    {
        return canvasMinX + laneLabelWidth + static_cast<float>(tick) * pixelsPerTick - timelineScrollX;
    }

    inline int screenYToTimelineLane(float screenY,
                                     float canvasMinY,
                                     float rulerHeight,
                                     float rowHeight,
                                     int laneCount)
    {
        const float safeRowHeight = std::max(1.0f, rowHeight);
        return std::clamp(static_cast<int>((screenY - canvasMinY - rulerHeight) / safeRowHeight),
                          0,
                          std::max(1, laneCount) - 1);
    }

    inline EditorRect noteVisualRect(int tick,
                                     int durationTicks,
                                     int stringIndex,
                                     float canvasMinX,
                                     float canvasMinY,
                                     float timelineScrollX,
                                     float laneLabelWidth,
                                     float pixelsPerTick,
                                     float rulerHeight,
                                     float rowHeight)
    {
        const float x = timelineTickToScreenX(tick, canvasMinX, timelineScrollX, laneLabelWidth, pixelsPerTick);
        const float noteVerticalPadding = std::max(8.0f, rowHeight * 0.12f);
        const float y = canvasMinY + rulerHeight + rowHeight * static_cast<float>(std::max(0, stringIndex)) + noteVerticalPadding;
        const float widthPx = std::max(32.0f, std::max(1, durationTicks) * pixelsPerTick - 6.0f);
        return {
            x,
            y,
            x + widthPx,
            y + rowHeight - noteVerticalPadding * 2.0f,
        };
    }

    inline bool containsNoteId(const std::vector<EditorNoteId> &ids, EditorNoteId id)
    {
        return std::find(ids.begin(), ids.end(), id) != ids.end();
    }

    inline void removeNoteId(std::vector<EditorNoteId> &ids, EditorNoteId id)
    {
        ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    }

    inline void addNoteIdIfMissing(std::vector<EditorNoteId> &ids, EditorNoteId id)
    {
        if (id != 0 && !containsNoteId(ids, id))
        {
            ids.push_back(id);
        }
    }

    inline std::vector<EditorNoteId> noteIdsForPart(const std::vector<EditorPartNoteRef> &notes,
                                                    std::string_view activePart)
    {
        std::vector<EditorNoteId> ids;
        for (const EditorPartNoteRef &note : notes)
        {
            if (note.id != 0 && note.part == activePart)
            {
                addNoteIdIfMissing(ids, note.id);
            }
        }
        return ids;
    }

    inline bool shouldHandleEditorShortcut(bool wantTextInput, bool anyItemActive, bool popupOpen)
    {
        return !wantTextInput && !anyItemActive && !popupOpen;
    }

    inline std::vector<EditorNoteId> noteIdsIntersectingRect(
        const std::vector<EditorSelectableNoteRect> &notes,
        const EditorRect &selectionRect)
    {
        std::vector<EditorNoteId> selected;
        for (const EditorSelectableNoteRect &note : notes)
        {
            if (note.id != 0 && rectsIntersect(note.rect, selectionRect))
            {
                selected.push_back(note.id);
            }
        }
        return selected;
    }

    inline std::vector<EditorNoteId> applySelection(
        std::vector<EditorNoteId> current,
        const std::vector<EditorNoteId> &hits,
        SelectionMode mode)
    {
        if (mode == SelectionMode::Replace)
        {
            current.clear();
        }

        for (const EditorNoteId id : hits)
        {
            if (id == 0)
            {
                continue;
            }
            if (mode == SelectionMode::Toggle && containsNoteId(current, id))
            {
                removeNoteId(current, id);
            }
            else
            {
                addNoteIdIfMissing(current, id);
            }
        }
        return current;
    }

    inline int snapDeltaTicks(int rawDeltaTicks, int snapTicks, bool snapEnabled = true)
    {
        if (!snapEnabled)
        {
            return rawDeltaTicks;
        }
        const int safeSnap = std::max(1, snapTicks);
        return static_cast<int>(std::round(static_cast<float>(rawDeltaTicks) / static_cast<float>(safeSnap))) * safeSnap;
    }

    inline int clampGroupDragDeltaAtZero(const std::vector<EditorDraggedNote> &notes, int desiredDeltaTicks)
    {
        if (notes.empty())
        {
            return desiredDeltaTicks;
        }
        int earliestTick = notes.front().originalTick;
        for (const EditorDraggedNote &note : notes)
        {
            earliestTick = std::min(earliestTick, note.originalTick);
        }
        return std::max(desiredDeltaTicks, -std::max(0, earliestTick));
    }

    inline int movedTickForDraggedNote(const EditorDraggedNote &note, int deltaTicks)
    {
        return std::max(0, note.originalTick + deltaTicks);
    }

    inline std::vector<EditorMovedNote> movedDraggedNotes(const std::vector<EditorDraggedNote> &notes, int deltaTicks)
    {
        std::vector<EditorMovedNote> moved;
        moved.reserve(notes.size());
        for (const EditorDraggedNote &note : notes)
        {
            moved.push_back({
                note.id,
                movedTickForDraggedNote(note, deltaTicks),
                1,
                note.stringIndex,
                note.fret,
            });
        }
        return moved;
    }

    inline std::vector<EditorClipboardNote> buildClipboardNotes(std::vector<EditorMovedNote> notes)
    {
        if (notes.empty())
        {
            return {};
        }
        std::sort(notes.begin(), notes.end(),
                  [](const EditorMovedNote &left, const EditorMovedNote &right)
                  {
                      if (left.tick != right.tick)
                      {
                          return left.tick < right.tick;
                      }
                      return left.id < right.id;
                  });
        const int anchorTick = notes.front().tick;
        std::vector<EditorClipboardNote> clipboard;
        clipboard.reserve(notes.size());
        for (const EditorMovedNote &note : notes)
        {
            clipboard.push_back({
                std::max(0, note.tick - anchorTick),
                std::max(1, note.durationTicks),
                note.stringIndex,
                note.fret,
            });
        }
        return clipboard;
    }

    inline std::vector<EditorMovedNote> pasteClipboardNotes(const std::vector<EditorClipboardNote> &clipboard,
                                                            int targetTick,
                                                            EditorNoteId firstId)
    {
        std::vector<EditorMovedNote> pasted;
        pasted.reserve(clipboard.size());
        for (size_t index = 0; index < clipboard.size(); ++index)
        {
            const EditorClipboardNote &note = clipboard[index];
            pasted.push_back({
                firstId + static_cast<EditorNoteId>(index),
                std::max(0, targetTick + note.relativeTick),
                std::max(1, note.durationTicks),
                note.stringIndex,
                note.fret,
            });
        }
        return pasted;
    }

    inline void pushEditorCommand(EditorCommandStack &stack, EditorSnapshotCommand command)
    {
        if (command.label.empty())
        {
            command.label = "Edit notes";
        }
        stack.undo.push_back(std::move(command));
        stack.redo.clear();
    }

    inline std::optional<EditorSnapshotCommand> popUndoCommand(EditorCommandStack &stack)
    {
        if (stack.undo.empty())
        {
            return std::nullopt;
        }
        EditorSnapshotCommand command = std::move(stack.undo.back());
        stack.undo.pop_back();
        stack.redo.push_back(command);
        return command;
    }

    inline std::optional<EditorSnapshotCommand> popRedoCommand(EditorCommandStack &stack)
    {
        if (stack.redo.empty())
        {
            return std::nullopt;
        }
        EditorSnapshotCommand command = std::move(stack.redo.back());
        stack.redo.pop_back();
        stack.undo.push_back(command);
        return command;
    }

    inline std::optional<int> parseNonNegativeInteger(std::string_view value)
    {
        if (value.empty())
        {
            return std::nullopt;
        }

        int parsed = 0;
        for (const char ch : value)
        {
            if (ch < '0' || ch > '9')
            {
                return std::nullopt;
            }
            parsed = parsed * 10 + (ch - '0');
        }
        return parsed;
    }

    inline int parseLengthSeconds(std::string_view value)
    {
        const size_t separator = value.find(':');
        if (separator == std::string_view::npos)
        {
            return 0;
        }

        const auto minutes = parseNonNegativeInteger(value.substr(0, separator));
        const auto seconds = parseNonNegativeInteger(value.substr(separator + 1));
        if (!minutes.has_value() || !seconds.has_value())
        {
            return 0;
        }
        return std::max(0, *minutes * 60 + *seconds);
    }

    inline std::string formatClock(double seconds)
    {
        const int clamped = std::max(0, static_cast<int>(std::round(seconds)));
        const int minutes = clamped / 60;
        const int remainder = clamped % 60;
        std::string formatted;
        if (minutes < 10)
        {
            formatted.push_back('0');
        }
        formatted += std::to_string(minutes);
        formatted.push_back(':');
        if (remainder < 10)
        {
            formatted.push_back('0');
        }
        formatted += std::to_string(remainder);
        return formatted;
    }

    inline int snapTickSize(int ticksPerBeat, int snapIndex)
    {
        const int safeTicksPerBeat = std::max(1, ticksPerBeat);
        const int divisor = kSnapDivisors[std::clamp(snapIndex, 0, static_cast<int>(kSnapDivisors.size()) - 1)];
        return std::max(1, safeTicksPerBeat / divisor);
    }

    inline int quantizeTick(int tick, int ticksPerBeat, int snapIndex)
    {
        const int snap = snapTickSize(ticksPerBeat, snapIndex);
        return std::max(0, static_cast<int>(std::round(static_cast<float>(tick) / static_cast<float>(snap))) * snap);
    }

    inline int timelineTotalBeats(int durationSeconds, int bpm, int beatsPerMeasure)
    {
        const int safeDuration = std::max(0, durationSeconds);
        const int safeBpm = std::max(1, bpm);
        const int safeBeatsPerMeasure = std::max(1, beatsPerMeasure);
        return std::max(safeBeatsPerMeasure * 4,
                        static_cast<int>(std::ceil(safeDuration * static_cast<double>(safeBpm) / 60.0)));
    }

    inline int timelineTotalTicks(int durationSeconds, int bpm, int beatsPerMeasure, int ticksPerBeat)
    {
        const int safeTicksPerBeat = std::max(1, ticksPerBeat);
        return std::max(safeTicksPerBeat, timelineTotalBeats(durationSeconds, bpm, beatsPerMeasure) * safeTicksPerBeat);
    }

    inline double clampTransportCursor(double seconds, int songDurationSeconds)
    {
        return std::clamp(seconds, 0.0, static_cast<double>(std::max(0, songDurationSeconds)));
    }

    inline double timelineTickFromSeconds(double seconds, int bpm, int ticksPerBeat, int songDurationSeconds)
    {
        return clampTransportCursor(seconds, songDurationSeconds) * static_cast<double>(std::max(1, bpm)) / 60.0 *
               static_cast<double>(std::max(1, ticksPerBeat));
    }

    inline double timelineSecondsFromTick(double tick, int bpm, int ticksPerBeat)
    {
        return std::max(0.0, tick) / static_cast<double>(std::max(1, ticksPerBeat)) *
               60.0 / static_cast<double>(std::max(1, bpm));
    }

    inline double chartSecondsFromAudioSeconds(double audioSeconds, int chartAudioOffsetMs)
    {
        return std::max(0.0, audioSeconds - static_cast<double>(chartAudioOffsetMs) / 1000.0);
    }

    inline double audioSecondsFromChartSeconds(double chartSeconds, int chartAudioOffsetMs)
    {
        return std::max(0.0, chartSeconds + static_cast<double>(chartAudioOffsetMs) / 1000.0);
    }
}
