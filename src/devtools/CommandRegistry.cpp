#include "devtools/CommandRegistry.h"

#include <algorithm>

namespace openchordix::devtools
{
    void CommandRegistry::registerCommand(std::unique_ptr<IDevCommand> command)
    {
        if (!command)
        {
            return;
        }
        commands_.push_back(std::move(command));
    }

    const IDevCommand *CommandRegistry::find(std::string_view name) const
    {
        auto it = std::find_if(commands_.begin(), commands_.end(), [&](const auto &cmd)
                               { return cmd && cmd->name() == name; });
        return it != commands_.end() ? it->get() : nullptr;
    }
}
