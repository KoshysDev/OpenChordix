#include "devtools/DevConsole.h"
#include "devtools/IDevCommand.h"

#include <memory>

namespace openchordix::devtools
{
    namespace
    {
        class ClearCommand final : public IDevCommand
        {
        public:
            std::string_view name() const override { return "clear"; }
            std::string_view help() const override { return "Clear the console log."; }

            void execute(DevConsole &console, std::span<const std::string_view> /*args*/) const override
            {
                console.clearLog();
            }
        };
    }

    std::unique_ptr<IDevCommand> makeClearCommand()
    {
        return std::make_unique<ClearCommand>();
    }
}
