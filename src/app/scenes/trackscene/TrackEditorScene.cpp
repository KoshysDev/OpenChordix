#include "trackscene/TrackEditorScene.h"

#include <imgui/imgui.h>

#include "trackscene/TrackEditorShared.h"
#include "trackscene/TrackEditorWidgets.h"
#include "track/ChartNotePreviewSynth.h"
#include "track/TrackAudioDuration.h"
#include "track/TrackCatalogFile.h"
#include "track/TrackFilePaths.h"

TrackEditorScene::TrackEditorScene(AnimatedUI &ui, std::string trackId)
    : ui_(ui),
      catalog_(std::make_unique<TrackCatalogFile>()),
      previewPlayer_(std::make_unique<openchordix::track::TrackPreviewPlayer>()),
      audioPicker_("track_audio_picker", "Audio File"),
      importPicker_("track_import_picker", "Import Chart")
{
    audioPicker_.setExtensions({".ogg", ".wav", ".mp3", ".flac", ".m4a", ".aac", ".opus"});
    importPicker_.setExtensions({".mid", ".midi", ".gp3", ".gp4", ".gp5", ".gpx", ".gp"});
    resetDraft();
    if (!trackId.empty())
    {
        loadTrack(trackId);
    }
}

void TrackEditorScene::render(float dt, const FrameInput & /*input*/, GraphicsContext & /*gfx*/, std::atomic<bool> & /*quitFlag*/)
{
    handleEditorShortcuts();

    if (!ImGui::GetIO().WantTextInput &&
        !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) &&
        ImGui::IsKeyPressed(ImGuiKey_Space, false))
    {
        togglePreviewPlayback();
    }
    previewPlayer_->update();

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar;

    if (ImGui::Begin("Track Editor", nullptr, flags))
    {
        drawBackground(screen);
        renderEditorMenuBar();
        renderEditorStatusStrip(screen);
        drawTimeline(screen, ImGui::GetCursorPosY() + track_editor::kEditorPanelSpacing, dt);
        drawBottomBar(screen);
        drawSongSetupModal();
        drawAddTuningModal();
        drawImportPreviewModal();
    }
    ImGui::End();

    if (auto chosen = audioPicker_.draw())
    {
        applyChosenAudioPath(*chosen);
    }
    if (auto chosen = importPicker_.draw())
    {
        loadImportSource(*chosen);
    }
}

TrackEditorScene::Action TrackEditorScene::consumeAction()
{
    const Action action = pendingAction_;
    pendingAction_ = Action::None;
    return action;
}

std::string TrackEditorScene::takeSavedTrackId()
{
    std::string trackId = std::move(savedTrackId_);
    savedTrackId_.clear();
    return trackId;
}

void TrackEditorScene::drawBackground(const ImVec2 &screen)
{
    ImDrawList *bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilledMultiColor(ImVec2(0.0f, 0.0f), screen,
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.06f, 0.09f, 1.0f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.06f, 0.10f, 1.0f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.08f, 0.12f, 1.0f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.03f, 0.04f, 0.08f, 1.0f)));
}

void TrackEditorScene::renderEditorMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        renderFileMenu();
        renderEditMenu();
        renderSongMenu();
        renderTrackMenu();
        renderImportMenu();
        renderTransportMenu();
        renderViewMenu();
        renderToolsMenu();
        renderHelpMenu();
        ImGui::EndMenuBar();
    }
}

void TrackEditorScene::renderEditorStatusStrip(const ImVec2 &screen)
{
    const std::string title = track_editor::trimCopy(draftTitle_.data());
    const std::string artist = track_editor::trimCopy(draftArtist_.data());
    const std::string summary = title.empty() ? "New Song" : title + (artist.empty() ? "" : " - " + artist);
    static constexpr std::array<const char *, 4> kToolLabels = {"Select", "Draw Note", "Erase", "Move"};
    const int toolIndex = static_cast<int>(editorTool_);

    ImGui::SetCursorPosX(track_editor::kEditorHorizontalMargin);
    ImGui::BeginChild("track_editor_status",
                      ImVec2(screen.x - track_editor::kEditorHorizontalMargin * 2.0f, track_editor::kStatusStripHeight),
                      false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 max(min.x + ImGui::GetWindowWidth(), min.y + track_editor::kStatusStripHeight);
    track_editor::drawPanelFrame(drawList, min, max, track_editor::kPanel);
    ImGui::SetCursorPos(ImVec2(12.0f, 10.0f));
    ImGui::TextColored(track_editor::kAccent, "%s", summary.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("| Part: %s  | Tool: %s  | Snap: %s  | Zoom: %.2fx  | Time: %s / %s  | Preview: %s",
                        currentPartName().c_str(),
                        kToolLabels[static_cast<size_t>(std::clamp(toolIndex, 0, 3))],
                        track_editor::kSnapLabels[static_cast<size_t>(std::clamp(snapIndex_, 0, 3))],
                        zoom_,
                        track_editor::formatClock(displayedCursorSeconds()).c_str(),
                        track_editor::formatClock(songDurationSeconds()).c_str(),
                        track_editor::formatClock(chart_.previewStartSeconds()).c_str());
    ImGui::EndChild();
}

#include "trackscene/TrackEditorSceneDialogsImpl.h"
#include "trackscene/TrackEditorSceneMenuImpl.h"
#include "trackscene/TrackEditorSceneTimelineImpl.h"
#include "trackscene/TrackEditorSceneBottomBarImpl.h"
#include "trackscene/TrackEditorSceneModelImpl.h"
#include "trackscene/TrackEditorSceneTransportImpl.h"
#include "trackscene/TrackEditorSceneImportImpl.h"
