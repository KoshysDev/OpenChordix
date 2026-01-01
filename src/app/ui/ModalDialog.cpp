#include "ui/ModalDialog.h"

#include <cfloat>

ModalDialog::ModalDialog(std::string id, std::string title) : id_(std::move(id)), title_(std::move(title))
{
}

ModalDialog ModalDialog::info(std::string id, std::string title, std::vector<std::string> body, std::string buttonLabel)
{
    ModalDialog dialog(std::move(id), std::move(title));
    dialog.setBody(std::move(body));
    dialog.setButtons({ModalButton{std::move(buttonLabel)}});
    return dialog;
}

ModalDialog ModalDialog::confirm(std::string id, std::string title, std::vector<std::string> body, std::string confirmLabel, std::string cancelLabel)
{
    ModalDialog dialog(std::move(id), std::move(title));
    dialog.setBody(std::move(body));
    dialog.setButtons({ModalButton{std::move(confirmLabel)}, ModalButton{std::move(cancelLabel)}});
    return dialog;
}

ModalDialog ModalDialog::confirmDiscard(std::string id, std::string title, std::vector<std::string> body, std::string applyLabel, std::string discardLabel, std::string stayLabel)
{
    ModalDialog dialog(std::move(id), std::move(title));
    dialog.setBody(std::move(body));
    dialog.setButtons({ModalButton{std::move(applyLabel)}, ModalButton{std::move(discardLabel)}, ModalButton{std::move(stayLabel)}});
    return dialog;
}

void ModalDialog::open()
{
    openRequested_ = true;
}

std::optional<std::size_t> ModalDialog::draw(const ImVec2 &minSize)
{
    const std::string popupLabel = title_ + "##" + id_;
    if (openRequested_)
    {
        ImGui::OpenPopup(popupLabel.c_str());
        openRequested_ = false;
    }

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, screen.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(minSize, ImVec2(FLT_MAX, FLT_MAX));

    std::optional<std::size_t> choice;
    if (ImGui::BeginPopupModal(popupLabel.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        for (const auto &line : body_)
        {
            ImGui::TextWrapped("%s", line.c_str());
        }

        if (!body_.empty())
        {
            ImGui::Spacing();
        }

        for (std::size_t i = 0; i < buttons_.size(); ++i)
        {
            if (i > 0)
            {
                ImGui::SameLine();
            }
            if (ImGui::Button(buttons_[i].label.c_str(), buttons_[i].size))
            {
                choice = i;
                ImGui::CloseCurrentPopup();
                break;
            }
        }

        ImGui::EndPopup();
    }
    return choice;
}
