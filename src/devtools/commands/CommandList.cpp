#include "devtools/commands/CommandList.h"

#include <memory>

#include "devtools/CommandRegistry.h"
#include "devtools/IDevCommand.h"

namespace openchordix::devtools
{
    std::unique_ptr<IDevCommand> makeHelpCommand();
    std::unique_ptr<IDevCommand> makeClearCommand();
    std::unique_ptr<IDevCommand> makeQuitCommand();

    void registerDefaultCommands(CommandRegistry &registry)
    {
        registry.registerCommand(makeHelpCommand());
        registry.registerCommand(makeClearCommand());
        registry.registerCommand(makeQuitCommand());
    }
}
