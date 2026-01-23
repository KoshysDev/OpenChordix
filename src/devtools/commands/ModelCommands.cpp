#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "devtools/CommandRegistry.h"
#include "devtools/DevConsole.h"
#include "devtools/IDevCommand.h"

namespace openchordix::devtools
{
    namespace
    {
        std::string joinTokens(std::span<const std::string_view> tokens, std::size_t start)
        {
            std::string result;
            for (std::size_t i = start; i < tokens.size(); ++i)
            {
                if (!result.empty())
                {
                    result.push_back(' ');
                }
                result.append(tokens[i]);
            }
            return result;
        }

        class ModelCommand final : public IDevCommand
        {
        public:
            ModelCommand(std::function<bool(std::string_view)> loadHandler,
                         std::function<void()> clearHandler,
                         std::function<std::string()> statusProvider)
                : loadHandler_(std::move(loadHandler)),
                  clearHandler_(std::move(clearHandler)),
                  statusProvider_(std::move(statusProvider))
            {
            }

            std::string_view name() const override { return "model"; }
            std::string_view help() const override { return "Model commands: model status | model load <path> | model clear"; }

            void execute(DevConsole &console, std::span<const std::string_view> args) const override
            {
                if (args.empty() || args.front() == "status")
                {
                    if (!statusProvider_)
                    {
                        console.addLog("Model status unavailable.");
                        return;
                    }
                    console.addLog(statusProvider_());
                    return;
                }

                if (args.front() == "clear")
                {
                    if (clearHandler_)
                    {
                        clearHandler_();
                        console.addLog("Model cleared.");
                    }
                    else
                    {
                        console.addLog("Model clearing unavailable.");
                    }
                    return;
                }

                if (args.front() == "load")
                {
                    if (args.size() < 2)
                    {
                        console.addLog("Usage: model load <path>");
                        return;
                    }
                    if (!loadHandler_)
                    {
                        console.addLog("Model loading unavailable.");
                        return;
                    }
                    std::string path = joinTokens(args, 1);
                    if (loadHandler_(path))
                    {
                        console.addLog("Model loaded: " + path);
                    }
                    else
                    {
                        console.addLog("Failed to load model: " + path);
                    }
                    return;
                }

                console.addLog("Unknown model command. Try: model status | model load <path> | model clear");
            }

        private:
            std::function<bool(std::string_view)> loadHandler_;
            std::function<void()> clearHandler_;
            std::function<std::string()> statusProvider_;
        };
    }

    void registerModelCommands(CommandRegistry &registry,
                               std::function<bool(std::string_view)> loadHandler,
                               std::function<void()> clearHandler,
                               std::function<std::string()> statusProvider)
    {
        registry.registerCommand(std::make_unique<ModelCommand>(std::move(loadHandler),
                                                                std::move(clearHandler),
                                                                std::move(statusProvider)));
    }
}
