#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "AnimatedUI.h"
#include "Scene.h"
#include "score/ScoreService.h"
#include "track/TrackCatalog.h"
#include "track/TrackPreviewPlayer.h"

class TrackSelectScene : public Scene
{
public:
    enum class Action
    {
        None,
        OpenCreateSong,
        EditSelectedSong
    };

    explicit TrackSelectScene(AnimatedUI &ui, std::string focusTrackId = {});

    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }
    Action consumeAction();
    std::string takeRequestedTrackId();

private:
    void drawBackground(const ImVec2 &screen);
    void drawHeader(const ImVec2 &screen);
    void drawMainPane(const ImVec2 &screen, float top, float height);
    void drawTrackRow(const TrackInfo &track, int index, float width);
    void updateFilter();
    bool removeSelectedSong();
    void applyFocusTrack();
    void syncPreviewForSelection();

    AnimatedUI &ui_;
    std::unique_ptr<TrackCatalog> catalog_;
    std::unique_ptr<TrackScoreService> scoreService_;
    std::unique_ptr<openchordix::track::TrackPreviewPlayer> previewPlayer_;
    Action pendingAction_ = Action::None;
    std::vector<int> filtered_;
    std::array<char, 64> search_{};
    std::string focusTrackId_;
    std::string previewSelectionId_;
    std::string previewTrackId_;
    std::string requestedTrackId_;
    std::string statusMessage_;
    int selectedIndex_ = 0;
    int selectedPart_ = 0;
    bool finished_ = false;
    bool showPlayNotice_ = false;
    bool confirmRemoveSong_ = false;
};
