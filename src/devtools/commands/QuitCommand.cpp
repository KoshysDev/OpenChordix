#include "devtools/DevConsole.h"
#include "devtools/IDevCommand.h"

#include <memory>

namespace openchordix::devtools
{
    namespace
    {
        class QuitCommand final : public IDevCommand
        {
        public:
            std::string_view name() const override { return "quit"; }
            std::string_view help() const override { return "Request app shutdown."; }

            void execute(DevConsole &console, std::span<const std::string_view> /*args*/) const override
            {
                console.addLog("Quit requested.");
                console.requestQuit();
            }
        };
    }

    std::unique_ptr<IDevCommand> makeQuitCommand()
    {
        return std::make_unique<QuitCommand>();
    }
}
