#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <vector>

#include "track/TrackEditorMath.h"

TEST_CASE("TrackEditor math formats and parses song clocks", "[track][editor]")
{
    using namespace openchordix::track::editor;

    CHECK(parseLengthSeconds("00:00") == 0);
    CHECK(parseLengthSeconds("01:40") == 100);
    CHECK(parseLengthSeconds("12:34") == 754);
    CHECK(parseLengthSeconds("bad") == 0);
    CHECK(parseLengthSeconds("12:xx") == 0);

    CHECK(formatClock(-2.0) == "00:00");
    CHECK(formatClock(100.1) == "01:40");
    CHECK(formatClock(754.4) == "12:34");
    CHECK(formatClock(754.6) == "12:35");
}

TEST_CASE("TrackEditor math quantizes ticks from snap settings", "[track][editor]")
{
    using namespace openchordix::track::editor;

    CHECK(snapTickSize(48, 0) == 48);
    CHECK(snapTickSize(48, 2) == 12);
    CHECK(snapTickSize(48, 99) == 6);

    CHECK(quantizeTick(-5, 48, 2) == 0);
    CHECK(quantizeTick(5, 48, 2) == 0);
    CHECK(quantizeTick(11, 48, 2) == 12);
    CHECK(quantizeTick(23, 48, 2) == 24);
}

TEST_CASE("TrackEditor math keeps timeline conversions stable", "[track][editor]")
{
    using namespace openchordix::track::editor;

    CHECK(timelineTotalBeats(90, 120, 4) == 180);
    CHECK(timelineTotalBeats(10, 120, 4) == 20);
    CHECK(timelineTotalTicks(90, 120, 4, 48) == 8640);

    CHECK(clampTransportCursor(-1.0, 120) == 0.0);
    CHECK(clampTransportCursor(140.0, 120) == 120.0);

    const double tick = timelineTickFromSeconds(30.0, 120, 48, 90);
    CHECK(tick == Catch::Approx(2880.0));
    CHECK(timelineSecondsFromTick(tick, 120, 48) == Catch::Approx(30.0));

    const double clampedTick = timelineTickFromSeconds(120.0, 120, 48, 90);
    CHECK(clampedTick == Catch::Approx(timelineTickFromSeconds(90.0, 120, 48, 90)));
}

TEST_CASE("TrackEditor math applies chart audio offset semantics", "[track][editor][timing]")
{
    using namespace openchordix::track::editor;

    CHECK(chartSecondsFromAudioSeconds(0.5, 0) == Catch::Approx(0.5));
    CHECK(chartSecondsFromAudioSeconds(0.5, 100) == Catch::Approx(0.4));
    CHECK(chartSecondsFromAudioSeconds(0.5, -100) == Catch::Approx(0.6));
    CHECK(audioSecondsFromChartSeconds(0.4, 100) == Catch::Approx(0.5));
    CHECK(audioSecondsFromChartSeconds(0.6, -100) == Catch::Approx(0.5));
}

TEST_CASE("TrackEditor selection math normalizes and intersects note rectangles", "[track][editor][selection]")
{
    using namespace openchordix::track::editor;

    const EditorRect leftToRight = normalizedRect(10.0f, 20.0f, 90.0f, 80.0f);
    CHECK(leftToRight.minX == Catch::Approx(10.0f));
    CHECK(leftToRight.minY == Catch::Approx(20.0f));
    CHECK(leftToRight.maxX == Catch::Approx(90.0f));
    CHECK(leftToRight.maxY == Catch::Approx(80.0f));

    const EditorRect rightToLeft = normalizedRect(90.0f, 80.0f, 10.0f, 20.0f);
    CHECK(rightToLeft.minX == Catch::Approx(10.0f));
    CHECK(rightToLeft.minY == Catch::Approx(20.0f));
    CHECK(rightToLeft.maxX == Catch::Approx(90.0f));
    CHECK(rightToLeft.maxY == Catch::Approx(80.0f));

    const std::vector<EditorSelectableNoteRect> notes = {
        {1, EditorRect{0.0f, 0.0f, 30.0f, 20.0f}},
        {2, EditorRect{40.0f, 10.0f, 80.0f, 30.0f}},
        {3, EditorRect{120.0f, 10.0f, 150.0f, 30.0f}},
    };

    const auto hits = noteIdsIntersectingRect(notes, normalizedRect(25.0f, 5.0f, 90.0f, 35.0f));
    REQUIRE(hits.size() == 2);
    CHECK(hits[0] == 1);
    CHECK(hits[1] == 2);

    const auto leftToRightHits = noteIdsIntersectingRect(notes, normalizedRect(35.0f, 0.0f, 100.0f, 40.0f));
    CHECK(leftToRightHits == std::vector<EditorNoteId>{2});

    const auto rightToLeftHits = noteIdsIntersectingRect(notes, normalizedRect(100.0f, 40.0f, 35.0f, 0.0f));
    CHECK(rightToLeftHits == std::vector<EditorNoteId>{2});

    const TimelineSelectionRange range = normalizedSelectionRange(960, 240, 5, 2);
    CHECK(range.minTick == 240);
    CHECK(range.maxTick == 960);
    CHECK(range.minLane == 2);
    CHECK(range.maxLane == 5);
    CHECK(noteIntersectsSelectionRange(900, 240, 4, range));
    CHECK(noteIntersectsSelectionRange(120, 240, 2, range));
    CHECK_FALSE(noteIntersectsSelectionRange(120, 60, 2, range));
    CHECK_FALSE(noteIntersectsSelectionRange(900, 240, 1, range));
}

TEST_CASE("TrackEditor selection math applies replace add and toggle modifiers", "[track][editor][selection]")
{
    using namespace openchordix::track::editor;

    CHECK(applySelection({9}, {1, 2}, SelectionMode::Replace) == std::vector<EditorNoteId>{1, 2});
    CHECK(applySelection({1}, {2, 3}, SelectionMode::Add) == std::vector<EditorNoteId>{1, 2, 3});
    CHECK(applySelection({1, 2}, {2, 3}, SelectionMode::Toggle) == std::vector<EditorNoteId>{1, 3});

    const std::vector<EditorPartNoteRef> partNotes = {
        {1, "Lead"},
        {2, "Bass"},
        {3, "Lead"},
        {0, "Lead"},
    };
    CHECK(noteIdsForPart(partNotes, "Lead") == std::vector<EditorNoteId>{1, 3});
    CHECK(noteIdsForPart(partNotes, "Bass") == std::vector<EditorNoteId>{2});
}

TEST_CASE("TrackEditor group drag snaps preserves spacing and clamps at zero", "[track][editor][selection]")
{
    using namespace openchordix::track::editor;

    const std::vector<EditorDraggedNote> notes = {
        {1, 480, 2, 5},
        {2, 960, 4, 7},
    };

    const int snappedDelta = snapDeltaTicks(251, 120);
    CHECK(snappedDelta == 240);
    CHECK(movedTickForDraggedNote(notes[0], snappedDelta) == 720);
    CHECK(movedTickForDraggedNote(notes[1], snappedDelta) == 1200);
    CHECK(movedTickForDraggedNote(notes[1], snappedDelta) - movedTickForDraggedNote(notes[0], snappedDelta) == 480);
    CHECK(notes[0].stringIndex == 2);
    CHECK(notes[0].fret == 5);

    const auto moved = movedDraggedNotes(notes, snappedDelta);
    REQUIRE(moved.size() == 2);
    CHECK(moved[0].tick == 720);
    CHECK(moved[1].tick == 1200);
    CHECK(moved[0].durationTicks == 1);
    CHECK(moved[0].stringIndex == 2);
    CHECK(moved[0].fret == 5);

    const int clampedDelta = clampGroupDragDeltaAtZero(notes, -900);
    CHECK(clampedDelta == -480);
    CHECK(movedTickForDraggedNote(notes[0], clampedDelta) == 0);
    CHECK(movedTickForDraggedNote(notes[1], clampedDelta) == 480);
}

TEST_CASE("TrackEditor auto-scroll velocity ramps and clamps", "[track][editor][selection]")
{
    using namespace openchordix::track::editor;

    const EditorRect timeline{100.0f, 0.0f, 500.0f, 300.0f};
    const TimelineAutoScrollConfig config{50.0f, 20.0f, 500.0f};

    CHECK(computeTimelineAutoScrollVelocity(250.0f, timeline, config) == Catch::Approx(0.0f));
    CHECK(computeTimelineAutoScrollVelocity(125.0f, timeline, config) < 0.0f);
    CHECK(computeTimelineAutoScrollVelocity(475.0f, timeline, config) > 0.0f);
    CHECK(std::abs(computeTimelineAutoScrollVelocity(101.0f, timeline, config)) >
          std::abs(computeTimelineAutoScrollVelocity(140.0f, timeline, config)));
    CHECK(computeTimelineAutoScrollVelocity(700.0f, timeline, config) == Catch::Approx(500.0f));
    CHECK(applyTimelineAutoScroll(5.0f, -500.0f, 1.0f, 1000.0f) == Catch::Approx(0.0f));
    CHECK(applyTimelineAutoScroll(980.0f, 500.0f, 1.0f, 1000.0f) == Catch::Approx(1000.0f));

    const int tickBeforeScroll = screenXToTimelineTick(480.0f, 100.0f, 0.0f, 52.0f, 0.5f);
    const int tickAfterScroll = screenXToTimelineTick(480.0f, 100.0f, 120.0f, 52.0f, 0.5f);
    CHECK(tickAfterScroll > tickBeforeScroll);
}

TEST_CASE("TrackEditor clipboard helpers preserve relative timing and target anchor", "[track][editor][clipboard]")
{
    using namespace openchordix::track::editor;

    const std::vector<EditorMovedNote> selected = {
        {2, 1440, 240, 3, 7},
        {1, 960, 480, 2, 5},
    };

    const auto clipboard = buildClipboardNotes(selected);
    REQUIRE(clipboard.size() == 2);
    CHECK(clipboard[0].relativeTick == 0);
    CHECK(clipboard[0].durationTicks == 480);
    CHECK(clipboard[1].relativeTick == 480);
    CHECK(clipboard[1].durationTicks == 240);

    const auto pasted = pasteClipboardNotes(clipboard, 2400, 10);
    REQUIRE(pasted.size() == 2);
    CHECK(pasted[0].id == 10);
    CHECK(pasted[0].tick == 2400);
    CHECK(pasted[0].durationTicks == 480);
    CHECK(pasted[1].tick == 2880);
    CHECK(pasted[1].stringIndex == 3);

    const auto clamped = pasteClipboardNotes(clipboard, -120, 20);
    CHECK(clamped[0].tick == 0);
    CHECK(clamped[1].tick == 360);
}

TEST_CASE("TrackEditor command stack groups undo redo and clears redo on new edit", "[track][editor][undo]")
{
    using namespace openchordix::track::editor;

    EditorCommandStack stack;
    pushEditorCommand(stack, EditorSnapshotCommand{"Move notes", {{1, 0, 1, 0, 3}, {2, 480, 1, 1, 5}}, {{1, 240, 1, 0, 3}, {2, 720, 1, 1, 5}}});
    REQUIRE(stack.undo.size() == 1);
    CHECK(stack.redo.empty());

    const auto undo = popUndoCommand(stack);
    REQUIRE(undo.has_value());
    CHECK(undo->label == "Move notes");
    REQUIRE(undo->before.size() == 2);
    CHECK(undo->before[0].tick == 0);
    CHECK(stack.undo.empty());
    REQUIRE(stack.redo.size() == 1);

    const auto redo = popRedoCommand(stack);
    REQUIRE(redo.has_value());
    CHECK(redo->after[1].tick == 720);
    REQUIRE(stack.undo.size() == 1);
    CHECK(stack.redo.empty());

    popUndoCommand(stack);
    REQUIRE(stack.redo.size() == 1);
    pushEditorCommand(stack, EditorSnapshotCommand{"Paste notes", {}, {{3, 960, 1, 2, 7}}});
    CHECK(stack.redo.empty());
    REQUIRE(stack.undo.size() == 1);
    CHECK(stack.undo.front().label == "Paste notes");
}

TEST_CASE("TrackEditor shortcut gate ignores text input active widgets and popups", "[track][editor][undo]")
{
    using namespace openchordix::track::editor;

    CHECK(shouldHandleEditorShortcut(false, false, false));
    CHECK_FALSE(shouldHandleEditorShortcut(true, false, false));
    CHECK_FALSE(shouldHandleEditorShortcut(false, true, false));
    CHECK_FALSE(shouldHandleEditorShortcut(false, false, true));
}

TEST_CASE("TrackEditor coordinate helpers map timeline positions and note rectangles", "[track][editor][selection]")
{
    using namespace openchordix::track::editor;

    constexpr float canvasMinX = 100.0f;
    constexpr float canvasMinY = 50.0f;
    constexpr float scrollX = 24.0f;
    constexpr float laneLabelWidth = 52.0f;
    constexpr float pixelsPerTick = 0.5f;
    constexpr float rulerHeight = 34.0f;
    constexpr float rowHeight = 40.0f;

    const float x = timelineTickToScreenX(480, canvasMinX, scrollX, laneLabelWidth, pixelsPerTick);
    CHECK(screenXToTimelineTick(x, canvasMinX, scrollX, laneLabelWidth, pixelsPerTick) == 480);
    CHECK(screenYToTimelineLane(canvasMinY + rulerHeight + rowHeight * 2.0f + 10.0f,
                                canvasMinY,
                                rulerHeight,
                                rowHeight,
                                6) == 2);

    const EditorRect rect = noteVisualRect(480,
                                           240,
                                           2,
                                           canvasMinX,
                                           canvasMinY,
                                           scrollX,
                                           laneLabelWidth,
                                           pixelsPerTick,
                                           rulerHeight,
                                           rowHeight);
    CHECK(rect.minX == Catch::Approx(x));
    CHECK(rect.maxX > rect.minX);
    CHECK(rect.minY > canvasMinY + rulerHeight + rowHeight * 2.0f);
    CHECK(rect.maxY < canvasMinY + rulerHeight + rowHeight * 3.0f);
}
