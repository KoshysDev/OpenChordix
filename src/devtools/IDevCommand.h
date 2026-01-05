#pragma once

#include <span>
#include <string_view>

namespace openchordix::devtools
{
    class DevConsole;

    class IDevCommand
    {
    public:
        virtual ~IDevCommand() = default;

        virtual std::string_view name() const = 0;
        virtual std::string_view help() const = 0;
        virtual void execute(DevConsole &console, std::span<const std::string_view> args) const = 0;
    };
}
