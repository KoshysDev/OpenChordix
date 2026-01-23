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
        class SceneListCommand final : public openchordix::devtools::IDevCommand
        {
        public:
            explicit SceneListCommand(std::function<std::vector<std::string>()> provider)
                : provider_(std::move(provider))
            {
            }

            std::string_view name() const override { return "scenes"; }
            std::string_view help() const override { return "List available scenes."; }

            void execute(openchordix::devtools::DevConsole &console, std::span<const std::string_view> /*args*/) const override
            {
                if (!provider_)
                {
                    console.addLog("No scenes registered.");
                    return;
                }
                auto scenes = provider_();
                if (scenes.empty())
                {
                    console.addLog("No scenes registered.");
                    return;
                }
                console.addLog("Scenes:");
                for (const auto &scene : scenes)
                {
                    console.addLog("  " + scene);
                }
            }

        private:
            std::function<std::vector<std::string>()> provider_;
        };

        class SceneLoadCommand final : public openchordix::devtools::IDevCommand
        {
        public:
            explicit SceneLoadCommand(std::function<bool(std::string_view)> handler)
                : handler_(std::move(handler))
            {
            }

            std::string_view name() const override { return "scene"; }
            std::string_view help() const override { return "Load a scene by name."; }

            void execute(openchordix::devtools::DevConsole &console, std::span<const std::string_view> args) const override
            {
                if (args.empty())
                {
                    console.addLog("Usage: scene <name>");
                    return;
                }
                if (!handler_)
                {
                    console.addLog("Scene loading is not available.");
                    return;
                }

                std::string_view sceneName = args.front();
                if (handler_(sceneName))
                {
                    console.addLog("Scene loaded: " + std::string(sceneName));
                }
                else
                {
                    console.addLog("Unknown scene: " + std::string(sceneName));
                }
            }

        private:
            std::function<bool(std::string_view)> handler_;
        };
    }

    void registerSceneCommands(openchordix::devtools::CommandRegistry &registry,
                               std::function<std::vector<std::string>()> listProvider,
                               std::function<bool(std::string_view)> loadHandler)
    {
        registry.registerCommand(std::make_unique<SceneListCommand>(std::move(listProvider)));
        registry.registerCommand(std::make_unique<SceneLoadCommand>(std::move(loadHandler)));
    }
}
