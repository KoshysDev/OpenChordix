#pragma once

#include <imgui/imgui.h>
#include <optional>
#include <string>
#include <vector>

struct ModalButton
{
    std::string label;
    ImVec2 size{140.0f, 0.0f};
};

class ModalDialog
{
public:
    ModalDialog(std::string id, std::string title);

    void setBody(std::vector<std::string> lines) { body_ = std::move(lines); }
    void setButtons(std::vector<ModalButton> buttons) { buttons_ = std::move(buttons); }
    void open();
    std::optional<std::size_t> draw(const ImVec2 &minSize = ImVec2(0.0f, 0.0f));

    static ModalDialog info(std::string id, std::string title, std::vector<std::string> body, std::string buttonLabel = "OK");
    static ModalDialog confirm(std::string id, std::string title, std::vector<std::string> body, std::string confirmLabel, std::string cancelLabel);
    static ModalDialog confirmDiscard(std::string id, std::string title, std::vector<std::string> body, std::string applyLabel, std::string discardLabel, std::string stayLabel);

private:
    std::string id_;
    std::string title_;
    std::vector<std::string> body_;
    std::vector<ModalButton> buttons_;
    bool openRequested_{false};
};
