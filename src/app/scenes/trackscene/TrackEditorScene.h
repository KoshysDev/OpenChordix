#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "AnimatedUI.h"
#include "Scene.h"
#include "track/TrackCatalog.h"
#include "track/TrackChartDocument.h"
#include "track/TrackEditorMath.h"
#include "track/TrackPreviewPlayer.h"
#include "track/TuningLibrary.h"
#include "track/import/ImporterRegistry.h"
#include "track/import/ImportTimingDebug.h"
#include "track/import/TrackImportApply.h"
#include "ui/FileDialog.h"

class TrackEditorScene : public Scene
{
public:
    enum class Action
    {
        None,
        Back
    };

    TrackEditorScene(AnimatedUI &ui, std::string trackId = {});

    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return false; }

    Action consumeAction();
    std::string takeSavedTrackId();

private:
    struct DraftPart
    {
        std::array<char, 64> name{};
        int stringCount = openchordix::track::kDefaultTrackStrings;
        std::vector<std::string> tuning;
    };

    struct ImportPartDraft
    {
        bool enabled = true;
        std::array<char, 64> name{};
        int stringCount = openchordix::track::kDefaultTrackStrings;
        std::vector<std::string> tuning;
    };

    struct NoteClipboard
    {
        std::string sourcePart;
        std::vector<TrackTabNote> notes;
    };

    struct NoteEditCommand
    {
        std::string label;
        std::vector<TrackTabNote> beforeNotes;
        std::vector<TrackTabNote> afterNotes;
        std::vector<openchordix::track::editor::EditorNoteId> beforeSelection;
        std::vector<openchordix::track::editor::EditorNoteId> afterSelection;
    };

    enum class EditorTool
    {
        Select,
        Draw,
        Erase,
        Move
    };

    void drawBackground(const ImVec2 &screen);
    void renderEditorMenuBar();
    void renderFileMenu();
    void renderEditMenu();
    void renderSongMenu();
    void renderTrackMenu();
    void renderImportMenu();
    void renderTransportMenu();
    void renderViewMenu();
    void renderToolsMenu();
    void renderHelpMenu();
    void renderEditorStatusStrip(const ImVec2 &screen);
    void drawSongSetupModal();
    void drawAddTuningModal();
    void drawImportPreviewModal();
    void drawTimeline(const ImVec2 &screen, float top, float dt);
    void drawBottomBar(const ImVec2 &screen);
    void resetDraft();
    void loadTrack(std::string_view trackId);
    bool saveDraft();
    void sortNotes();
    std::vector<TrackPart> collectParts() const;
    TrackPart buildPartFromDraft(const DraftPart &draft) const;
    TrackPart currentPart() const;
    std::string currentPartName() const;
    int currentPartStringCount() const;
    std::vector<std::string> currentStringLabels() const;
    std::filesystem::path trackRootDirectory() const;
    std::filesystem::path resolveAudioBrowseDirectory() const;
    std::filesystem::path currentChartPath(const TrackInfo &track) const;
    std::filesystem::path currentResolvedAudioPath() const;
    void applyChosenAudioPath(const std::filesystem::path &path);
    void openImportPicker(const std::vector<std::string> &extensions);
    void loadImportSource(const std::filesystem::path &path);
    void applyImportPreview();
    void ensureSelectedPartIsValid();
    void initializeDraftPart(DraftPart &part, std::string_view name);
    void loadDraftPart(DraftPart &draft, const TrackPart &part);
    void setDraftPartStringCount(DraftPart &draft, int stringCount);
    void applyPresetToDraftPart(DraftPart &draft, const openchordix::track::TuningPreset &preset);
    const openchordix::track::TuningPreset *matchingPresetForDraftPart(const DraftPart &draft) const;
    void openAddTuningDialog(int targetPartIndex);
    int snapTickSize() const;
    int quantizeTick(int tick) const;
    void removeSelectedNote();
    void refreshDurationFromAudio();
    TrackInfo buildTrackDraft() const;
    int songDurationSeconds() const;
    double clampTransportCursor(double seconds) const;
    double displayedCursorSeconds() const;
    int timelineTotalBeats() const;
    int timelineTotalTicks() const;
    openchordix::track::TempoMap currentTempoMap() const;
    double timelineTickFromSeconds(double seconds) const;
    double timelineSecondsFromTick(double tick) const;
    void pausePreviewPlayback();
    void togglePreviewPlayback();
    void stopPreviewToStart();
    void requestTimelineSync();
    void syncTimelineScrollToSeconds(double seconds, float viewportWidth = -1.0f);
    bool startPreviewFromCursor();
    void refreshTimingPreview();
    void ensureNoteEditorIds();
    int noteIndexByEditorId(openchordix::track::editor::EditorNoteId id) const;
    bool isNoteSelected(openchordix::track::editor::EditorNoteId id) const;
    void clearNoteSelection();
    void setPrimarySelectedNote(openchordix::track::editor::EditorNoteId id);
    void syncSelectedNoteIndex();
    int commitDraggedSelectionMove(int deltaTicks);
    bool hasSelectedNotes() const;
    bool activePartHasNotes() const;
    void copySelectedNotes();
    void cutSelectedNotes();
    void pasteCopiedNotes();
    void deleteSelectedNotes();
    void selectAllCurrentPartNotes();
    bool canUndoNoteEdit() const;
    bool canRedoNoteEdit() const;
    void undoNoteEdit();
    void redoNoteEdit();
    void pushNoteEditCommand(std::string label,
                             std::vector<TrackTabNote> beforeNotes,
                             std::vector<openchordix::track::editor::EditorNoteId> beforeSelection);
    void restoreNoteEditState(const std::vector<TrackTabNote> &notes,
                              const std::vector<openchordix::track::editor::EditorNoteId> &selection);
    void handleEditorShortcuts();

    AnimatedUI &ui_;
    std::unique_ptr<TrackCatalog> catalog_;
    std::unique_ptr<openchordix::track::TrackPreviewPlayer> previewPlayer_;
    openchordix::track::TuningLibrary tuningLibrary_;
    FileDialog audioPicker_;
    FileDialog importPicker_;
    openchordix::track::imports::ImporterRegistry importerRegistry_;
    TrackChartDocument chart_;
    Action pendingAction_ = Action::None;
    int draftBpm_ = 120;
    int snapIndex_ = 2;
    int selectedPartIndex_ = 0;
    int selectedNoteIndex_ = -1;
    float zoom_ = 1.0f;
    int durationSeconds_ = 90;
    float timelineScrollX_ = 0.0f;
    float timelineViewportWidth_ = 0.0f;
    double transportCursorSeconds_ = 0.0;
    std::array<char, 128> draftTitle_{};
    std::array<char, 128> draftArtist_{};
    std::array<char, 128> draftSource_{};
    std::array<char, 128> draftMapper_{};
    std::array<char, 256> draftAudioFile_{};
    std::array<char, 64> draftTuningName_{};
    int draftTuningStringCount_ = openchordix::track::kDefaultTrackStrings;
    std::vector<std::string> draftTuningNotes_;
    int draftTuningTargetPartIndex_ = -1;
    std::vector<DraftPart> draftParts_;
    std::optional<openchordix::track::imports::ImportedSong> importedSong_;
    std::vector<ImportPartDraft> importDraftParts_;
    openchordix::track::imports::ImportProgress importProgress_;
    std::filesystem::path importSourcePath_;
    openchordix::track::imports::ImportMergeMode importMergeMode_ =
        openchordix::track::imports::ImportMergeMode::Additive;
    openchordix::track::imports::ImportPlacementOptions importPlacement_;
    int importTimingPartIndex_ = 0;
    EditorTool editorTool_ = EditorTool::Draw;
    std::string editingTrackId_;
    std::string savedTrackId_;
    std::string statusMessage_;
    std::string importError_;
    std::string importDetectedFormat_;
    std::string lastImportStatus_;
    bool openSongSetup_ = false;
    bool openAddTuning_ = false;
    bool openImportPreview_ = false;
    bool applyImportedTempo_ = true;
    bool showTimingDiagnostics_ = false;
    bool metronomeEnabled_ = false;
    bool noteClicksEnabled_ = false;
    bool songMuted_ = false;
    bool metronomeMuted_ = false;
    bool notePreviewMuted_ = false;
    float songVolume_ = 1.0f;
    float metronomeVolume_ = 0.65f;
    float notePreviewVolume_ = 0.75f;
    openchordix::track::editor::EditorNoteId nextEditorNoteId_ = 1;
    std::vector<openchordix::track::editor::EditorNoteId> selectedNoteIds_;
    bool selectionDragActive_ = false;
    bool selectionDragExceededThreshold_ = false;
    ImVec2 selectionStartMouse_{};
    ImVec2 selectionCurrentMouse_{};
    int selectionStartTick_ = 0;
    int selectionCurrentTick_ = 0;
    int selectionStartLane_ = 0;
    int selectionCurrentLane_ = 0;
    openchordix::track::editor::SelectionMode selectionMode_ =
        openchordix::track::editor::SelectionMode::Replace;
    bool draggingNote_ = false;
    openchordix::track::editor::EditorNoteId draggingNoteId_ = 0;
    int dragStartMouseTick_ = 0;
    int currentDragDeltaTicks_ = 0;
    std::vector<openchordix::track::editor::EditorDraggedNote> draggedSelectionOriginalNotes_;
    NoteClipboard noteClipboard_;
    std::vector<NoteEditCommand> undoStack_;
    std::vector<NoteEditCommand> redoStack_;
    bool noteInspectorEditActive_ = false;
    std::vector<TrackTabNote> noteInspectorBeforeNotes_;
    std::vector<openchordix::track::editor::EditorNoteId> noteInspectorBeforeSelection_;
    bool scrubbingTransport_ = false;
    bool scrubResumePlayback_ = false;
    bool timelineSyncPending_ = true;
};
