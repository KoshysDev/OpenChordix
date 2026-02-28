#include "ui/FileDialog.h"
#include "ui/IconGlyphs.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace
{
    std::string normalizeExtension(std::string ext)
    {
        if (ext.empty())
        {
            return ext;
        }
        if (ext.front() != '.')
        {
            ext.insert(ext.begin(), '.');
        }
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return ext;
    }

    std::filesystem::path homeDirectory()
    {
#if defined(_WIN32)
        if (const char *home = std::getenv("USERPROFILE"))
        {
            return std::filesystem::path(home);
        }
        if (const char *drive = std::getenv("HOMEDRIVE"))
        {
            if (const char *path = std::getenv("HOMEPATH"))
            {
                return std::filesystem::path(std::string(drive) + std::string(path));
            }
        }
        return std::filesystem::current_path();
#else
        if (const char *home = std::getenv("HOME"))
        {
            return std::filesystem::path(home);
        }
        return std::filesystem::current_path();
#endif
    }

    std::vector<std::filesystem::path> splitPath(const std::filesystem::path &path)
    {
        std::vector<std::filesystem::path> parts;
        std::filesystem::path current;
        for (const auto &part : path)
        {
            current /= part;
            parts.push_back(current);
        }
        return parts;
    }

    bool isValidDirectory(const std::filesystem::path &path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
    }

    bool drawEntryRow(const char *id, std::string_view label, std::string_view kind)
    {
        ImGui::PushID(id);
        const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 rowStart = ImGui::GetCursorScreenPos();
        bool clicked = ImGui::Selectable("##entry", false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, rowHeight));

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        float iconSize = rowHeight * 0.65f;
        ImVec2 iconPos(rowStart.x + 6.0f, rowStart.y + (rowHeight - iconSize) * 0.5f);
        openchordix::ui::drawIconGlyph(drawList, iconPos, iconSize, kind);

        ImVec2 textPos(iconPos.x + iconSize + 8.0f, rowStart.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f);
        drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), label.data());

        ImGui::PopID();
        return clicked;
    }
}

FileDialog::FileDialog(std::string id, std::string title)
    : id_(std::move(id)),
      title_(std::move(title))
{
    browseDir_ = std::filesystem::current_path();
    auto dirString = browseDir_.string();
    std::strncpy(browsePath_.data(), dirString.c_str(), browsePath_.size() - 1);
    browsePath_[browsePath_.size() - 1] = '\0';
}

void FileDialog::setDirectory(const std::filesystem::path &dir)
{
    std::error_code ec;
    if (std::filesystem::exists(dir, ec) && std::filesystem::is_directory(dir, ec))
    {
        browseDir_ = dir;
        auto dirString = browseDir_.string();
        std::strncpy(browsePath_.data(), dirString.c_str(), browsePath_.size() - 1);
        browsePath_[browsePath_.size() - 1] = '\0';
    }
}

void FileDialog::setExtensions(std::vector<std::string> extensions)
{
    extensions_.clear();
    for (auto &ext : extensions)
    {
        auto normalized = normalizeExtension(std::move(ext));
        if (!normalized.empty())
        {
            extensions_.push_back(std::move(normalized));
        }
    }
    std::sort(extensions_.begin(), extensions_.end());
    extensions_.erase(std::unique(extensions_.begin(), extensions_.end()), extensions_.end());
}

void FileDialog::open()
{
    openRequested_ = true;
}

std::optional<std::filesystem::path> FileDialog::draw(const ImVec2 &listSize)
{
    const std::string popupLabel = title_ + "##" + id_;
    if (openRequested_)
    {
        ImGui::OpenPopup(popupLabel.c_str());
        openRequested_ = false;
    }

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, screen.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    std::optional<std::filesystem::path> selection;
    if (ImGui::BeginPopupModal(popupLabel.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!extensions_.empty())
        {
            std::string allowed = "Allowed: ";
            for (std::size_t i = 0; i < extensions_.size(); ++i)
            {
                if (i > 0)
                {
                    allowed += ", ";
                }
                allowed += extensions_[i];
            }
            ImGui::TextUnformatted(allowed.c_str());
            ImGui::Separator();
        }

        ImGui::InputText("Directory", browsePath_.data(), browsePath_.size());
        ImGui::SameLine();
        if (openchordix::ui::iconButton("##go", "go", "Go to directory"))
        {
            std::filesystem::path candidate(browsePath_.data());
            if (isValidDirectory(candidate))
            {
                browseDir_ = candidate;
            }
        }
        ImGui::SameLine();
        if (openchordix::ui::iconButton("##up", "up", "Parent directory"))
        {
            std::error_code ec;
            auto parent = browseDir_.parent_path();
            if (!parent.empty() && std::filesystem::exists(parent, ec))
            {
                browseDir_ = parent;
            }
        }

        auto dirString = browseDir_.string();
        if (dirString != browsePath_.data())
        {
            std::strncpy(browsePath_.data(), dirString.c_str(), browsePath_.size() - 1);
            browsePath_[browsePath_.size() - 1] = '\0';
        }

        struct Entry
        {
            std::filesystem::path path;
            bool isDir;
        };

        std::vector<Entry> entries;
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(browseDir_, ec))
        {
            if (ec)
            {
                break;
            }
            bool isDir = entry.is_directory(ec);
            if (isDir)
            {
                entries.push_back({entry.path(), true});
            }
            else
            {
                if (extensions_.empty())
                {
                    entries.push_back({entry.path(), false});
                    continue;
                }
                auto ext = normalizeExtension(entry.path().extension().string());
                if (std::find(extensions_.begin(), extensions_.end(), ext) != extensions_.end())
                {
                    entries.push_back({entry.path(), false});
                }
            }
        }

        std::sort(entries.begin(), entries.end(),
                  [](const Entry &a, const Entry &b)
                  {
                      if (a.isDir != b.isDir)
                      {
                          return a.isDir > b.isDir;
                      }
                      return a.path.filename().string() < b.path.filename().string();
                  });

        const float sidebarWidth = 220.0f;
        if (ImGui::BeginTable("FileDialogLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, sidebarWidth);
            ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::BeginChild("FileDialogSidebar", ImVec2(0.0f, listSize.y), true);
            ImGui::TextUnformatted("Places");
            ImGui::Separator();
            if (drawEntryRow("place_home", "Home", "home"))
            {
                browseDir_ = homeDirectory();
            }
            if (drawEntryRow("place_root", "Root", "root"))
            {
                browseDir_ = browseDir_.root_path().empty() ? std::filesystem::path("/") : browseDir_.root_path();
            }
            if (drawEntryRow("place_current", "Current", "current"))
            {
                browseDir_ = std::filesystem::current_path();
            }

            ImGui::Spacing();
            ImGui::TextUnformatted("Path");
            ImGui::Separator();
            for (const auto &part : splitPath(browseDir_))
            {
                std::string label = part.filename().string();
                if (label.empty())
                {
                    label = part.string();
                }
                if (drawEntryRow(part.string().c_str(), label, "folder"))
                {
                    browseDir_ = part;
                }
            }

            ImGui::Spacing();
            ImGui::TextUnformatted("Folders");
            ImGui::Separator();
            for (const auto &entry : entries)
            {
                if (!entry.isDir)
                {
                    continue;
                }
                std::string label = entry.path.filename().string();
                if (drawEntryRow(entry.path.string().c_str(), label, "folder"))
                {
                    browseDir_ = entry.path;
                }
            }
            ImGui::EndChild();

            ImGui::TableNextColumn();
            ImGui::BeginChild("FileDialogList", listSize, true);
            for (const auto &entry : entries)
            {
                std::string label = entry.path.filename().string();
                std::string_view kind = entry.isDir ? "folder" : "file";
                if (drawEntryRow(entry.path.string().c_str(), label, kind))
                {
                    if (entry.isDir)
                    {
                        browseDir_ = entry.path;
                    }
                    else
                    {
                        selection = entry.path;
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndChild();

            ImGui::EndTable();
        }

        ImGui::Separator();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    return selection;
}
