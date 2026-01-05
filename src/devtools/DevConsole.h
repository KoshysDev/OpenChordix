#pragma once

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <imgui/imgui.h>

#include "devtools/CommandRegistry.h"

namespace openchordix::devtools
{

    class DevConsole
    {
    public:
        explicit DevConsole(bool enabled = false);

        void setQuitCallback(std::function<void()> callback);
        void requestQuit();

        void updateToggle(bool togglePressed);
        void render();

        bool enabled() const { return enabled_; }
        bool visible() const { return visible_; }
        void addLog(std::string message);
        void clearLog();

        CommandRegistry &registry() { return registry_; }
        const CommandRegistry &registry() const { return registry_; }

    private:
        static int inputCallback(ImGuiInputTextCallbackData *data);
        int handleInputCallback(ImGuiInputTextCallbackData *data);
        void executeLine(std::string_view line);

        bool enabled_ = false;
        bool visible_ = false;
        bool lastToggleDown_ = false;
        bool scrollToBottom_ = false;
        bool reclaimFocus_ = false;
        int historyPos_ = -1;

        std::function<void()> quitCallback_;
        CommandRegistry registry_{};
        std::vector<std::string> log_;
        std::vector<std::string> history_;

        char inputBuf_[256]{};
    };

}
