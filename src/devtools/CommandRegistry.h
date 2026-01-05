#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "devtools/IDevCommand.h"

namespace openchordix::devtools
{
    class CommandRegistry
    {
    public:
        void registerCommand(std::unique_ptr<IDevCommand> command);
        const IDevCommand *find(std::string_view name) const;
        const std::vector<std::unique_ptr<IDevCommand>> &commands() const { return commands_; }

    private:
        std::vector<std::unique_ptr<IDevCommand>> commands_;
    };
}
