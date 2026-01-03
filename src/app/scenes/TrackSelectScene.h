#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "AnimatedUI.h"
#include "Scene.h"
#include "score/ScoreService.h"
#include "track/TrackCatalog.h"

class TrackSelectScene : public Scene
{
public:
    explicit TrackSelectScene(AnimatedUI &ui);

    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    void drawBackground(const ImVec2 &screen);
    void drawHeader(const ImVec2 &screen);
    void drawMainPane(const ImVec2 &screen, float top, float height);
    void drawTrackRow(const TrackInfo &track, int index, float width);
    void updateFilter();

    AnimatedUI &ui_;
    std::unique_ptr<TrackCatalog> catalog_;
    std::unique_ptr<TrackScoreService> scoreService_;
    std::vector<int> filtered_;
    std::array<char, 64> search_{};
    int selectedIndex_ = 0;
    int selectedPart_ = 0;
    bool finished_ = false;
    bool showPlayNotice_ = false;
};
