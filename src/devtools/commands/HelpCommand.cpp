#include "devtools/CommandRegistry.h"
#include "devtools/DevConsole.h"
#include "devtools/IDevCommand.h"

#include <memory>

namespace openchordix::devtools
{
    namespace
    {
        class HelpCommand final : public IDevCommand
        {
        public:
            std::string_view name() const override { return "help"; }
            std::string_view help() const override { return "List available commands."; }

            void execute(DevConsole &console, std::span<const std::string_view> /*args*/) const override
            {
                console.addLog("Commands:");
                for (const auto &cmd : console.registry().commands())
                {
                    if (cmd)
                    {
                        console.addLog("  " + std::string(cmd->name()) + " - " + std::string(cmd->help()));
                    }
                }
            }
        };
    }

    std::unique_ptr<IDevCommand> makeHelpCommand()
    {
        return std::make_unique<HelpCommand>();
    }
}
