#include "devtools/DevConsole.h"

#include <algorithm>
#include <cctype>

namespace openchordix::devtools
{
    namespace
    {
        std::vector<std::string_view> splitTokens(std::string_view line)
        {
            std::vector<std::string_view> tokens;
            std::size_t pos = 0;
            while (pos < line.size())
            {
                while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])))
                {
                    ++pos;
                }
                if (pos >= line.size())
                {
                    break;
                }
                std::size_t start = pos;
                while (pos < line.size() && !std::isspace(static_cast<unsigned char>(line[pos])))
                {
                    ++pos;
                }
                tokens.emplace_back(line.substr(start, pos - start));
            }
            return tokens;
        }
    }

    DevConsole::DevConsole(bool enabled)
        : enabled_(enabled), visible_(enabled)
    {
        if (enabled_)
        {
            addLog("Dev console enabled. Press `~` to toggle.");
        }
    }

    void DevConsole::setQuitCallback(std::function<void()> callback)
    {
        quitCallback_ = std::move(callback);
    }

    void DevConsole::requestQuit()
    {
        if (quitCallback_)
        {
            quitCallback_();
        }
    }

    void DevConsole::updateToggle(bool togglePressed)
    {
        if (!enabled_)
        {
            return;
        }

        if (togglePressed && !lastToggleDown_)
        {
            visible_ = !visible_;
            reclaimFocus_ = visible_;
        }
        lastToggleDown_ = togglePressed;
    }

    void DevConsole::render()
    {
        if (!enabled_ || !visible_)
        {
            return;
        }

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.03f, 0.04f, 0.97f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.03f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.07f, 0.09f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.10f, 0.13f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.12f, 0.16f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(520.0f, 360.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Dev Console", &visible_))
        {
            ImGui::End();
            ImGui::PopStyleColor(5);
            return;
        }

        ImGui::TextUnformatted("Type 'help' for commands.");
        ImGui::Separator();

        const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto &line : log_)
        {
            ImGui::TextUnformatted(line.c_str());
        }
        if (scrollToBottom_)
        {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom_ = false;
        }
        ImGui::EndChild();

        const float inputPadding = 5.0f;
        const float inputWidth = std::max(0.0f, ImGui::GetContentRegionAvail().x - (inputPadding * 2.0f));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + inputPadding);
        ImGui::PushItemWidth(inputWidth);
        if (reclaimFocus_)
        {
            ImGui::SetKeyboardFocusHere();
            reclaimFocus_ = false;
        }
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("##console_input", inputBuf_, sizeof(inputBuf_), flags, &DevConsole::inputCallback, this))
        {
            executeLine(inputBuf_);
            inputBuf_[0] = '\0';
            reclaimFocus_ = true;
        }
        ImGui::PopItemWidth();

        if (reclaimFocus_)
        {
            ImGui::SetKeyboardFocusHere();
            reclaimFocus_ = false;
        }

        ImGui::End();
        ImGui::PopStyleColor(5);
    }

    void DevConsole::addLog(std::string message)
    {
        log_.push_back(std::move(message));
        scrollToBottom_ = true;
    }

    void DevConsole::clearLog()
    {
        log_.clear();
        scrollToBottom_ = true;
    }

    int DevConsole::inputCallback(ImGuiInputTextCallbackData *data)
    {
        auto *console = static_cast<DevConsole *>(data->UserData);
        return console->handleInputCallback(data);
    }

    int DevConsole::handleInputCallback(ImGuiInputTextCallbackData *data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
        {
            const int previousPos = historyPos_;
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                if (historyPos_ == -1)
                {
                    historyPos_ = static_cast<int>(history_.size()) - 1;
                }
                else if (historyPos_ > 0)
                {
                    --historyPos_;
                }
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                if (historyPos_ != -1)
                {
                    ++historyPos_;
                    if (historyPos_ >= static_cast<int>(history_.size()))
                    {
                        historyPos_ = -1;
                    }
                }
            }

            if (previousPos != historyPos_)
            {
                const char *historyStr = (historyPos_ >= 0) ? history_[historyPos_].c_str() : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, historyStr);
            }
        }
        return 0;
    }

    void DevConsole::executeLine(std::string_view line)
    {
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front())))
        {
            line.remove_prefix(1);
        }
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back())))
        {
            line.remove_suffix(1);
        }

        if (line.empty())
        {
            return;
        }

        addLog("> " + std::string(line));

        if (history_.empty() || history_.back() != line)
        {
            history_.emplace_back(line);
        }
        historyPos_ = -1;

        auto tokens = splitTokens(line);
        if (tokens.empty())
        {
            return;
        }

        const IDevCommand *command = registry_.find(tokens.front());
        if (!command)
        {
            addLog("Unknown command: " + std::string(tokens.front()));
            return;
        }

        std::span<const std::string_view> args(tokens.data() + 1, tokens.size() - 1);
        command->execute(*this, args);
    }

}
