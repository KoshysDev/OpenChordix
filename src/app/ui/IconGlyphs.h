#pragma once

#include <imgui/imgui.h>
#include <string_view>

namespace openchordix::ui
{
    ImU32 iconFillColor(std::string_view glyph);
    void drawIconGlyph(ImDrawList *drawList, ImVec2 pos, float size, std::string_view glyph);
    bool iconButton(const char *id, std::string_view glyph, const char *tooltip = nullptr);
}
