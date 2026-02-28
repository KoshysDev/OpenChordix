#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <imgui/imgui.h>

class FileDialog
{
public:
    FileDialog(std::string id, std::string title);

    void setDirectory(const std::filesystem::path &dir);
    void setExtensions(std::vector<std::string> extensions);
    void open();

    std::optional<std::filesystem::path> draw(const ImVec2 &listSize = ImVec2(520.0f, 360.0f));

    const std::filesystem::path &directory() const { return browseDir_; }

private:
    std::string id_;
    std::string title_;
    std::vector<std::string> extensions_;
    std::filesystem::path browseDir_;
    std::array<char, 512> browsePath_{};
    bool openRequested_{false};
};
