#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace openchordix::track
{
    inline std::string trimCopy(std::string_view value)
    {
        const auto first = std::find_if_not(
            value.begin(),
            value.end(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            });
        const auto last = std::find_if_not(
            value.rbegin(),
            value.rend(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            }).base();
        return first >= last ? std::string{} : std::string(first, last);
    }
}
