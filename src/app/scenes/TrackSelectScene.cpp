#include "TrackSelectScene.h"

#include <algorithm>
#include <cstdio>
#include <utility>

#include <imgui/imgui.h>

#include "score/ScoreServiceMemory.h"
#include "track/TrackCatalogFile.h"
#include "track/TrackFilePaths.h"

namespace
{
    const ImVec4 kAccent = ImVec4(0.34f, 0.78f, 0.98f, 1.0f);
    const ImVec4 kMuted = ImVec4(0.70f, 0.78f, 0.90f, 1.0f);
    const ImVec4 kPanelBorder = ImVec4(0.20f, 0.26f, 0.34f, 0.9f);
}

TrackSelectScene::TrackSelectScene(AnimatedUI &ui, std::string focusTrackId)
    : ui_(ui),
      focusTrackId_(std::move(focusTrackId))
{
    catalog_ = std::make_unique<TrackCatalogFile>();
    scoreService_ = std::make_unique<TrackScoreServiceMemory>();
    previewPlayer_ = std::make_unique<openchordix::track::TrackPreviewPlayer>();
    updateFilter();
    applyFocusTrack();
}

void TrackSelectScene::render(float /*dt*/, const FrameInput & /*input*/, GraphicsContext & /*gfx*/, std::atomic<bool> & /*quitFlag*/)
{
    previewPlayer_->update();
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("Track Select", nullptr, flags))
    {
        drawBackground(screen);

        const float pad = 24.0f;
        ImGui::SetCursorPos(ImVec2(pad, pad));
        drawHeader(screen);

        ImGui::Dummy(ImVec2(0.0f, 12.0f));
        const float contentTop = ImGui::GetCursorPosY();
        const float contentHeight = screen.y - contentTop - pad;
        drawMainPane(screen, contentTop, contentHeight);

        if (confirmRemoveSong_)
        {
            ImGui::OpenPopup("Delete Song");
            confirmRemoveSong_ = false;
        }

        if (ImGui::BeginPopupModal("Delete Song", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Delete selected song from the database?");
            ImGui::Separator();
            if (ImGui::Button("Delete", ImVec2(140.0f, 0.0f)))
            {
                removeSelectedSong();
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
    ImGui::End();
}

TrackSelectScene::Action TrackSelectScene::consumeAction()
{
    const Action action = pendingAction_;
    pendingAction_ = Action::None;
    return action;
}

std::string TrackSelectScene::takeRequestedTrackId()
{
    std::string trackId = std::move(requestedTrackId_);
    requestedTrackId_.clear();
    return trackId;
}

void TrackSelectScene::drawBackground(const ImVec2 &screen)
{
    ImDrawList *bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilledMultiColor(ImVec2(0, 0), screen,
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.06f, 0.09f, 1.0f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.10f, 0.16f, 1.0f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.05f, 0.10f, 1.0f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.03f, 0.04f, 0.08f, 1.0f)));

    ImVec2 artMin(screen.x * 0.05f, screen.y * 0.1f);
    ImVec2 artMax(screen.x * 0.95f, screen.y * 0.9f);
    bg->AddRectFilledMultiColor(artMin, artMax,
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.12f, 0.18f, 0.26f, 0.55f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.14f, 0.22f, 0.55f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f, 0.10f, 0.18f, 0.55f)),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.12f, 0.20f, 0.55f)));
    bg->AddRectFilled(artMin, artMax, ImGui::ColorConvertFloat4ToU32(ImVec4(0.02f, 0.03f, 0.05f, 0.45f)), 24.0f);
}

void TrackSelectScene::drawHeader(const ImVec2 &screen)
{
    const float pad = 24.0f;
    ImGui::BeginGroup();
    if (ui_.button("< Back", ImVec2(110.0f, 42.0f)))
    {
        finished_ = true;
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(pad + 130.0f);
    ImGui::TextColored(kAccent, "Track Select");
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::SetCursorPosX(screen.x - 120.0f);
    ImGui::BeginGroup();
    ImGui::TextDisabled("Session");
    ImGui::Text("Local");
    ImGui::EndGroup();
}

void TrackSelectScene::drawMainPane(const ImVec2 &screen, float top, float height)
{
    const float pad = 24.0f;
    ImGui::SetCursorPos(ImVec2(pad, top));
    const ImVec2 panelSize(screen.x - pad * 2.0f, height);
    ImGui::BeginChild("track_select_panel", panelSize, false, ImGuiWindowFlags_NoScrollbar);

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 panelMin = ImGui::GetWindowPos();
    ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
    dl->AddRectFilled(panelMin, panelMax, ImGui::GetColorU32(ImVec4(0.06f, 0.08f, 0.12f, 0.88f)), 18.0f);
    dl->AddRect(panelMin, panelMax, ImGui::GetColorU32(kPanelBorder), 18.0f, 0, 1.5f);

    const float innerPad = 20.0f;
    const float rightWidth = std::clamp(panelSize.x * 0.32f, 260.0f, 380.0f);
    const float leftWidth = panelSize.x - innerPad * 3.0f - rightWidth;
    const ImVec2 leftPos(innerPad, innerPad);
    const ImVec2 rightPos(leftPos.x + leftWidth + innerPad, innerPad);

    const auto &tracks = catalog_->tracks();
    const bool hasTracks = !tracks.empty();
    const bool hasFiltered = !filtered_.empty();

    ImGui::SetCursorPos(leftPos);
    if (!hasTracks || !hasFiltered)
    {
        if (!hasTracks)
        {
            ImGui::TextDisabled("No songs found. Create a song to start building your local catalog.");
        }
        else
        {
            ImGui::TextDisabled("No tracks match current filter.");
        }
    }
    else
    {
        selectedIndex_ = std::clamp(selectedIndex_, 0, static_cast<int>(tracks.size()) - 1);
        if (std::find(filtered_.begin(), filtered_.end(), selectedIndex_) == filtered_.end())
        {
            selectedIndex_ = filtered_.front();
        }

        const TrackInfo &track = tracks[selectedIndex_];
        syncPreviewForSelection();
        if (selectedPart_ < 0 || selectedPart_ >= static_cast<int>(track.parts.size()))
        {
            selectedPart_ = 0;
        }

        const ImVec2 heroSize(leftWidth, 240.0f);
        const ImVec2 heroMin = ImGui::GetCursorScreenPos();
        const ImVec2 heroMax(heroMin.x + heroSize.x, heroMin.y + heroSize.y);
        ImGui::Dummy(heroSize);
        dl->AddRectFilledMultiColor(heroMin, heroMax,
                                    ImGui::GetColorU32(ImVec4(0.12f, 0.18f, 0.26f, 0.95f)),
                                    ImGui::GetColorU32(ImVec4(0.08f, 0.12f, 0.18f, 0.95f)),
                                    ImGui::GetColorU32(ImVec4(0.05f, 0.08f, 0.12f, 0.95f)),
                                    ImGui::GetColorU32(ImVec4(0.09f, 0.14f, 0.20f, 0.95f)));
        dl->AddRect(heroMin, heroMax, ImGui::GetColorU32(kPanelBorder), 16.0f, 0, 2.0f);

        const ImVec2 textPos(heroMin.x + 20.0f, heroMin.y + 24.0f);
        dl->AddText(textPos, ImGui::GetColorU32(kAccent), track.title.c_str());
        dl->AddText(ImVec2(textPos.x, textPos.y + 28.0f), ImGui::GetColorU32(ImVec4(0.92f, 0.96f, 1.0f, 1.0f)),
                    track.artist.c_str());
        dl->AddText(ImVec2(textPos.x, textPos.y + 56.0f), ImGui::GetColorU32(kMuted), track.source.c_str());
        dl->AddText(ImVec2(textPos.x, textPos.y + 82.0f), ImGui::GetColorU32(kMuted), "Mapped by:");
        dl->AddText(ImVec2(textPos.x + 88.0f, textPos.y + 82.0f), ImGui::GetColorU32(ImVec4(0.86f, 0.90f, 0.96f, 1.0f)),
                    track.mapper.c_str());
        if (!track.directory.empty())
        {
            dl->AddText(ImVec2(textPos.x, textPos.y + 108.0f), ImGui::GetColorU32(kMuted), track.directory.c_str());
        }

        char metaText[96];
        std::snprintf(metaText, sizeof(metaText), "%d BPM  |  %s", track.bpm, track.length.c_str());
        dl->AddText(ImVec2(textPos.x, textPos.y + 134.0f), ImGui::GetColorU32(kMuted), metaText);

        float partsX = textPos.x;
        float partsY = textPos.y + 162.0f;
        for (size_t i = 0; i < track.parts.size(); ++i)
        {
            const TrackPart &part = track.parts[i];
            ImVec2 labelSize = ImGui::CalcTextSize(part.name.c_str());
            float buttonW = std::clamp(labelSize.x + 24.0f, 96.0f, 160.0f);
            if (partsX + buttonW > heroMax.x - 16.0f)
            {
                partsX = textPos.x;
                partsY += 32.0f;
            }
            ImGui::SetCursorScreenPos(ImVec2(partsX, partsY));
            ImGui::PushID(static_cast<int>(i));
            const bool selected = selectedPart_ == static_cast<int>(i);
            if (selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(kAccent.x, kAccent.y, kAccent.z, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(kAccent.x * 0.8f, kAccent.y * 0.8f, kAccent.z * 0.8f, 1.0f));
            }
            if (ImGui::Button(part.name.c_str(), ImVec2(buttonW, 24.0f)))
            {
                selectedPart_ = static_cast<int>(i);
            }
            if (selected)
            {
                ImGui::PopStyleColor(3);
            }
            ImGui::PopID();
            partsX += buttonW + 8.0f;
        }

        ImGui::SetCursorPos(ImVec2(leftPos.x, leftPos.y + heroSize.y + 18.0f));
        const ImVec2 buttonSize(180.0f, 42.0f);
        if (ui_.button("Play", buttonSize))
        {
            showPlayNotice_ = true;
        }
        ImGui::SameLine();
        if (ui_.button(previewPlayer_->isPlaying() && previewTrackId_ == track.id ? "Stop Preview" : "Replay Preview", buttonSize))
        {
            if (previewPlayer_->isPlaying() && previewTrackId_ == track.id)
            {
                previewPlayer_->stop();
                previewTrackId_.clear();
            }
            else
            {
                previewSelectionId_.clear();
                previewTrackId_.clear();
                syncPreviewForSelection();
            }
        }
        ImGui::SameLine();
        if (ui_.button("Edit Selected", buttonSize))
        {
            requestedTrackId_ = track.id;
            pendingAction_ = Action::EditSelectedSong;
        }

        ImGui::SetCursorPos(ImVec2(leftPos.x, leftPos.y + heroSize.y + 68.0f));
        ImGui::BeginChild("score_panel", ImVec2(leftWidth, 190.0f), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextDisabled("Top Scores");
        ImGui::Separator();

        auto drawScoreRows = [&](const std::vector<ScoreEntry> &scores)
        {
            ImDrawList *scoreDl = ImGui::GetWindowDrawList();
            for (size_t i = 0; i < scores.size(); ++i)
            {
                const ScoreEntry &entry = scores[i];
                ImGui::PushID(static_cast<int>(i));
                ImVec2 rowSize(ImGui::GetContentRegionAvail().x, 36.0f);
                ImVec2 rowMin = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("score_row", rowSize);
                ImVec2 rowMax(rowMin.x + rowSize.x, rowMin.y + rowSize.y);
                scoreDl->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(ImVec4(0.10f, 0.14f, 0.20f, 0.85f)), 10.0f);
                scoreDl->AddRect(rowMin, rowMax, ImGui::GetColorU32(kPanelBorder), 10.0f, 0, 1.0f);

                ImVec2 badgeMin(rowMin.x + 8.0f, rowMin.y + 6.0f);
                ImVec2 badgeMax(rowMin.x + 30.0f, rowMin.y + 28.0f);
                scoreDl->AddRectFilled(badgeMin, badgeMax, ImGui::GetColorU32(kAccent), 6.0f);
                char placeText[8];
                std::snprintf(placeText, sizeof(placeText), "%d", entry.place);
                ImVec2 placeSize = ImGui::CalcTextSize(placeText);
                scoreDl->AddText(ImVec2(badgeMin.x + (badgeMax.x - badgeMin.x - placeSize.x) * 0.5f,
                                        badgeMin.y + (badgeMax.y - badgeMin.y - placeSize.y) * 0.5f),
                                 ImGui::GetColorU32(ImVec4(0.02f, 0.04f, 0.06f, 1.0f)), placeText);

                scoreDl->AddText(ImVec2(rowMin.x + 40.0f, rowMin.y + 8.0f),
                                 ImGui::GetColorU32(ImVec4(0.92f, 0.96f, 1.0f, 1.0f)), entry.name.c_str());
                char comboText[32];
                std::snprintf(comboText, sizeof(comboText), "%dx", entry.combo);
                ImVec2 comboSize = ImGui::CalcTextSize(comboText);
                scoreDl->AddText(ImVec2(rowMax.x - comboSize.x - 90.0f, rowMin.y + 8.0f),
                                 ImGui::GetColorU32(kMuted), comboText);
                char scoreText[32];
                std::snprintf(scoreText, sizeof(scoreText), "%d", entry.score);
                ImVec2 scoreSize = ImGui::CalcTextSize(scoreText);
                scoreDl->AddText(ImVec2(rowMax.x - scoreSize.x - 16.0f, rowMin.y + 8.0f),
                                 ImGui::GetColorU32(kAccent), scoreText);

                ImGui::PopID();
            }
        };

        if (ImGui::BeginTabBar("score_tabs"))
        {
            const std::string partName = track.parts.empty() ? "" : track.parts[selectedPart_].name;
            if (ImGui::BeginTabItem("Local"))
            {
                drawScoreRows(scoreService_->scoresFor(track, partName, ScoreCategory::Local));
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Online"))
            {
                drawScoreRows(scoreService_->scoresFor(track, partName, ScoreCategory::Online));
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Country"))
            {
                drawScoreRows(scoreService_->scoresFor(track, partName, ScoreCategory::Country));
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        if (showPlayNotice_)
        {
            ImGui::SetCursorPos(ImVec2(leftPos.x, leftPos.y + heroSize.y + 250.0f));
            ImGui::TextColored(kMuted, "Gameplay not implemented yet.");
        }
        if (!statusMessage_.empty())
        {
            ImGui::SetCursorPos(ImVec2(leftPos.x, leftPos.y + heroSize.y + 272.0f));
            ImGui::TextColored(kMuted, "%s", statusMessage_.c_str());
        }
        else
        {
            ImGui::SetCursorPos(ImVec2(leftPos.x, leftPos.y + heroSize.y + 272.0f));
            ImGui::TextColored(kMuted, "%s", previewPlayer_->status().c_str());
        }
    }

    ImGui::SetCursorPos(rightPos);
    ImGui::BeginChild("track_right_panel", ImVec2(rightWidth, panelSize.y - innerPad * 2.0f), false, ImGuiWindowFlags_NoScrollbar);
    ImVec2 rightMin = ImGui::GetWindowPos();
    ImVec2 rightMax(rightMin.x + rightWidth, rightMin.y + panelSize.y - innerPad * 2.0f);
    dl->AddRectFilled(rightMin, rightMax, ImGui::GetColorU32(ImVec4(0.07f, 0.10f, 0.14f, 0.92f)), 14.0f);
    dl->AddRect(rightMin, rightMax, ImGui::GetColorU32(kPanelBorder), 14.0f, 0, 1.0f);

    ImGui::SetCursorPos(ImVec2(16.0f, 16.0f));
    ImGui::TextColored(kAccent, "Song List");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("Search");
    ImGui::PushItemWidth(-70.0f);
    if (ImGui::InputTextWithHint("##track_search", "Title or artist", search_.data(), search_.size()))
    {
        updateFilter();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear"))
    {
        search_.fill('\0');
        updateFilter();
    }
    ImGui::Spacing();
    if (ImGui::Button("New Song", ImVec2(-1.0f, 34.0f)))
    {
        pendingAction_ = Action::OpenCreateSong;
    }
    ImGui::Spacing();

    ImGui::BeginChild("track_right_list", ImVec2(0, panelSize.y - innerPad * 2.0f - 220.0f), false, ImGuiWindowFlags_NoScrollbar);
    if (filtered_.empty())
    {
        ImGui::TextDisabled(hasTracks ? "No results." : "No songs found.");
    }
    else
    {
        const float listWidth = ImGui::GetContentRegionAvail().x;
        for (int index : filtered_)
        {
            drawTrackRow(tracks[index], index, listWidth);
        }
    }
    ImGui::EndChild();

    ImGui::BeginDisabled(filtered_.empty());
    if (ImGui::Button("Delete Selected Song", ImVec2(-1.0f, 34.0f)))
    {
        confirmRemoveSong_ = true;
    }
    ImGui::EndDisabled();

    ImGui::EndChild();
    ImGui::EndChild();
}

void TrackSelectScene::drawTrackRow(const TrackInfo &track, int index, float width)
{
    const float height = 64.0f;
    ImGui::PushID(index);
    ImGui::InvisibleButton("track_row", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const bool selected = selectedIndex_ == index;
    ImVec4 fill = ImVec4(0.10f, 0.12f, 0.18f, 0.92f);
    ImVec4 edge = kPanelBorder;
    if (selected)
    {
        fill = ImVec4(0.16f, 0.24f, 0.34f, 1.0f);
        edge = kAccent;
    }
    else if (hovered)
    {
        fill = ImVec4(0.13f, 0.18f, 0.26f, 0.98f);
    }

    dl->AddRectFilled(min, max, ImGui::GetColorU32(fill), 12.0f);
    dl->AddRect(min, max, ImGui::GetColorU32(edge), 12.0f, 0, 1.2f);
    dl->AddRectFilled(ImVec2(min.x, min.y), ImVec2(min.x + 6.0f, max.y),
                      ImGui::GetColorU32(selected ? kAccent : ImVec4(0.12f, 0.18f, 0.26f, 1.0f)), 12.0f, ImDrawFlags_RoundCornersLeft);

    ImVec2 titlePos(min.x + 16.0f, min.y + 10.0f);
    dl->AddText(titlePos, ImGui::GetColorU32(ImVec4(0.92f, 0.96f, 1.0f, 1.0f)), track.title.c_str());
    ImVec2 artistPos(min.x + 16.0f, min.y + 32.0f);
    dl->AddText(artistPos, ImGui::GetColorU32(kMuted), track.artist.c_str());

    char bpmText[32];
    std::snprintf(bpmText, sizeof(bpmText), "%d BPM", track.bpm);
    ImVec2 bpmSize = ImGui::CalcTextSize(bpmText);
    dl->AddText(ImVec2(max.x - bpmSize.x - 16.0f, min.y + 10.0f), ImGui::GetColorU32(kMuted), bpmText);
    dl->AddText(ImVec2(max.x - 16.0f - ImGui::CalcTextSize(track.length.c_str()).x, min.y + 32.0f),
                ImGui::GetColorU32(kMuted), track.length.c_str());

    if (clicked)
    {
        selectedIndex_ = index;
        selectedPart_ = 0;
    }
    ImGui::PopID();
    ImGui::Spacing();
}

bool TrackSelectScene::removeSelectedSong()
{
    const auto &tracks = catalog_->tracks();
    if (tracks.empty() || filtered_.empty())
    {
        statusMessage_ = "No song selected.";
        return false;
    }

    selectedIndex_ = std::clamp(selectedIndex_, 0, static_cast<int>(tracks.size()) - 1);
    const std::string selectedId = tracks[selectedIndex_].id;
    if (selectedId.empty())
    {
        statusMessage_ = "Selected song has no valid id.";
        return false;
    }

    if (!catalog_->removeTrack(selectedId))
    {
        statusMessage_ = "Failed to remove selected song.";
        return false;
    }

    updateFilter();
    showPlayNotice_ = false;
    statusMessage_ = "Song removed.";
    return true;
}

void TrackSelectScene::updateFilter()
{
    filtered_ = catalog_->filter(search_.data());
    if (filtered_.empty())
    {
        selectedIndex_ = 0;
        selectedPart_ = 0;
        return;
    }

    if (std::find(filtered_.begin(), filtered_.end(), selectedIndex_) == filtered_.end())
    {
        selectedIndex_ = filtered_.front();
        selectedPart_ = 0;
    }
}

void TrackSelectScene::applyFocusTrack()
{
    if (focusTrackId_.empty())
    {
        return;
    }

    const auto &tracks = catalog_->tracks();
    const auto it = std::find_if(
        tracks.begin(),
        tracks.end(),
        [&](const TrackInfo &track)
        {
            return track.id == focusTrackId_;
        });
    if (it == tracks.end())
    {
        focusTrackId_.clear();
        return;
    }

    const int index = static_cast<int>(std::distance(tracks.begin(), it));
    if (std::find(filtered_.begin(), filtered_.end(), index) != filtered_.end())
    {
        selectedIndex_ = index;
        selectedPart_ = 0;
    }
    focusTrackId_.clear();
}

void TrackSelectScene::syncPreviewForSelection()
{
    if (filtered_.empty())
    {
        previewPlayer_->stop();
        previewSelectionId_.clear();
        previewTrackId_.clear();
        return;
    }

    const auto &tracks = catalog_->tracks();
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(tracks.size()))
    {
        return;
    }

    const TrackInfo &track = tracks[selectedIndex_];
    if (previewSelectionId_ == track.id)
    {
        return;
    }

    previewSelectionId_ = track.id;
    previewPlayer_->stop();
    previewTrackId_ = track.id;

    const std::filesystem::path audioPath =
        openchordix::track::resolveAudioPath(track, catalog_->storagePath().parent_path());
    if (audioPath.empty())
    {
        return;
    }

    if (!previewPlayer_->play(audioPath, track.previewStartSeconds))
    {
        previewTrackId_.clear();
    }
}
