#include "TrackSelectScene.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <imgui/imgui.h>

#include "score/ScoreServiceMemory.h"
#include "track/TrackCatalogFile.h"

namespace
{
    const ImVec4 kAccent = ImVec4(0.34f, 0.78f, 0.98f, 1.0f);
    const ImVec4 kMuted = ImVec4(0.70f, 0.78f, 0.90f, 1.0f);
    const ImVec4 kPanelBorder = ImVec4(0.20f, 0.26f, 0.34f, 0.9f);

    std::string trimCopy(std::string_view value)
    {
        size_t first = 0;
        while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
        {
            ++first;
        }
        size_t last = value.size();
        while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
        {
            --last;
        }
        return std::string(value.substr(first, last - first));
    }

    std::vector<TrackPart> parseParts(std::string_view raw)
    {
        std::vector<TrackPart> parts;
        std::string token;
        token.reserve(raw.size());

        auto flushToken = [&]()
        {
            std::string partName = trimCopy(token);
            if (!partName.empty())
            {
                parts.push_back(TrackPart{partName});
            }
            token.clear();
        };

        for (char ch : raw)
        {
            if (ch == ',' || ch == ';')
            {
                flushToken();
                continue;
            }
            token.push_back(ch);
        }
        flushToken();

        if (parts.empty())
        {
            parts.push_back(TrackPart{"Lead Guitar"});
        }
        return parts;
    }
}

TrackSelectScene::TrackSelectScene(AnimatedUI &ui, bool startInCreateMode)
    : ui_(ui)
{
    catalog_ = std::make_unique<TrackCatalogFile>();
    scoreService_ = std::make_unique<TrackScoreServiceMemory>();
    resetSongDraft();
    updateFilter();
    if (startInCreateMode)
    {
        openCreateSongEditor();
    }
}

void TrackSelectScene::render(float /*dt*/, const FrameInput & /*input*/, GraphicsContext & /*gfx*/, std::atomic<bool> & /*quitFlag*/)
{
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

        if (openSongEditor_)
        {
            ImGui::OpenPopup("Song Editor");
            openSongEditor_ = false;
        }
        drawSongEditorPopup();

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
            ImGui::TextDisabled("No Songs found, try importing existing ones, or create your own.");
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
        const ImVec2 buttonSize(200.0f, 42.0f);
        if (ui_.button("Play", buttonSize))
        {
            showPlayNotice_ = true;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Preview unavailable");

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

    ImGui::BeginChild("track_right_list", ImVec2(0, panelSize.y - innerPad * 2.0f - 220.0f), false, ImGuiWindowFlags_NoScrollbar);
    if (filtered_.empty())
    {
        ImGui::TextDisabled(hasTracks ? "No results." : "No Songs found.");
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

    if (!filtered_.empty() && ImGui::Button("Delete Selected Song", ImVec2(-1.0f, 34.0f)))
    {
        confirmRemoveSong_ = true;
    }

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

void TrackSelectScene::drawSongEditorPopup()
{
    ImGui::SetNextWindowSize(ImVec2(640.0f, 0.0f), ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal("Song Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::TextDisabled(editorEditMode_
                            ? "Edit selected song entry."
                            : "Create a song entry for the local database.");
    ImGui::Separator();

    ImGui::InputTextWithHint("Title", "Song title", draftTitle_.data(), draftTitle_.size());
    ImGui::InputTextWithHint("Artist", "Artist name", draftArtist_.data(), draftArtist_.size());
    ImGui::InputTextWithHint("Source", "Album/Pack/Genre", draftSource_.data(), draftSource_.size());
    ImGui::InputTextWithHint("Mapper", "Your name", draftMapper_.data(), draftMapper_.size());

    ImGui::InputInt("BPM", &draftBpm_);
    if (draftBpm_ < 1)
    {
        draftBpm_ = 1;
    }

    ImGui::InputTextWithHint("Parts", "Lead Guitar, Rhythm Guitar, Bass", draftParts_.data(), draftParts_.size());
    ImGui::TextDisabled("Chart folder and chart file are created automatically in Songs/.");

    ImGui::Separator();
    if (ImGui::Button(editorEditMode_ ? "Save Changes" : "Create Song", ImVec2(160.0f, 0.0f)))
    {
        if (addSongFromDraft())
        {
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(120.0f, 0.0f)))
    {
        if (editorEditMode_ && !filtered_.empty())
        {
            const auto &tracks = catalog_->tracks();
            if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(tracks.size()))
            {
                loadDraftFromTrack(tracks[selectedIndex_]);
            }
        }
        else
        {
            resetSongDraft();
        }
        editorStatus_.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
    {
        editorEditMode_ = false;
        editingSongId_.clear();
        ImGui::CloseCurrentPopup();
    }

    if (!editorStatus_.empty())
    {
        ImGui::Spacing();
        ImGui::TextColored(kMuted, "%s", editorStatus_.c_str());
    }

    ImGui::EndPopup();
}

void TrackSelectScene::openCreateSongEditor()
{
    editorEditMode_ = false;
    editingSongId_.clear();
    editorStatus_.clear();
    resetSongDraft();
    openSongEditor_ = true;
}

void TrackSelectScene::loadDraftFromTrack(const TrackInfo &track)
{
    resetSongDraft();

    auto copyText = [](auto &buffer, const std::string &value)
    {
        std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
    };

    copyText(draftTitle_, track.title);
    copyText(draftArtist_, track.artist);
    copyText(draftSource_, track.source);
    copyText(draftMapper_, track.mapper);

    std::string partsJoined;
    for (size_t i = 0; i < track.parts.size(); ++i)
    {
        if (i != 0)
        {
            partsJoined += ", ";
        }
        partsJoined += track.parts[i].name;
    }
    copyText(draftParts_, partsJoined);
    draftBpm_ = std::max(1, track.bpm);
}

void TrackSelectScene::resetSongDraft()
{
    draftTitle_.fill('\0');
    draftArtist_.fill('\0');
    draftSource_.fill('\0');
    draftMapper_.fill('\0');
    draftParts_.fill('\0');

    draftBpm_ = 120;
    std::snprintf(draftParts_.data(), draftParts_.size(), "Lead Guitar, Rhythm Guitar, Bass");
}

bool TrackSelectScene::addSongFromDraft()
{
    TrackInfo track;
    track.title = trimCopy(draftTitle_.data());
    track.artist = trimCopy(draftArtist_.data());
    track.source = trimCopy(draftSource_.data());
    track.mapper = trimCopy(draftMapper_.data());
    track.bpm = draftBpm_;
    track.parts = parseParts(draftParts_.data());

    if (track.title.empty())
    {
        editorStatus_ = "Song title is required.";
        return false;
    }
    if (track.artist.empty())
    {
        editorStatus_ = "Artist is required.";
        return false;
    }
    if (track.bpm <= 0)
    {
        editorStatus_ = "BPM must be greater than 0.";
        return false;
    }

    std::string persistedId;
    if (editorEditMode_)
    {
        if (editingSongId_.empty())
        {
            editorStatus_ = "No song selected for edit.";
            return false;
        }
        if (!catalog_->updateTrack(editingSongId_, track))
        {
            editorStatus_ = "Failed to update song. Verify DB file permissions and fields.";
            return false;
        }
        persistedId = editingSongId_;
        editorStatus_ = "Song updated.";
    }
    else
    {
        if (!catalog_->addTrack(track))
        {
            editorStatus_ = "Failed to add song. Verify DB file permissions and fields.";
            return false;
        }
        if (!catalog_->tracks().empty())
        {
            persistedId = catalog_->tracks().back().id;
        }
        editorStatus_ = "Song added.";
    }

    search_.fill('\0');
    updateFilter();
    if (!persistedId.empty())
    {
        const auto &tracks = catalog_->tracks();
        for (size_t i = 0; i < tracks.size(); ++i)
        {
            if (tracks[i].id == persistedId)
            {
                selectedIndex_ = static_cast<int>(i);
                selectedPart_ = 0;
                break;
            }
        }
    }

    editorEditMode_ = false;
    editingSongId_.clear();
    resetSongDraft();
    return true;
}

bool TrackSelectScene::removeSelectedSong()
{
    const auto &tracks = catalog_->tracks();
    if (tracks.empty() || filtered_.empty())
    {
        editorStatus_ = "No song selected.";
        return false;
    }

    selectedIndex_ = std::clamp(selectedIndex_, 0, static_cast<int>(tracks.size()) - 1);
    const std::string selectedId = tracks[selectedIndex_].id;
    if (selectedId.empty())
    {
        editorStatus_ = "Selected song has no valid id.";
        return false;
    }

    if (!catalog_->removeTrack(selectedId))
    {
        editorStatus_ = "Failed to remove selected song.";
        return false;
    }

    if (editingSongId_ == selectedId)
    {
        editorEditMode_ = false;
        editingSongId_.clear();
    }

    updateFilter();
    showPlayNotice_ = false;
    editorStatus_ = "Song removed.";
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
