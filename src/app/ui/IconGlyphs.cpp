#include "ui/IconGlyphs.h"

namespace openchordix::ui
{
    namespace
    {
        enum class IconKind
        {
            Folder,
            File,
            Home,
            Root,
            Current,
            Go,
            Up,
            Unknown
        };

        struct GlyphEntry
        {
            std::string_view name;
            IconKind kind;
        };

        constexpr GlyphEntry kGlyphs[] = {
            {"folder", IconKind::Folder},
            {"file", IconKind::File},
            {"home", IconKind::Home},
            {"root", IconKind::Root},
            {"current", IconKind::Current},
            {"go", IconKind::Go},
            {"up", IconKind::Up},
        };

        IconKind resolveGlyph(std::string_view glyph)
        {
            for (const auto &entry : kGlyphs)
            {
                if (entry.name == glyph)
                {
                    return entry.kind;
                }
            }
            return IconKind::Unknown;
        }
    }

    ImU32 iconFillColor(std::string_view glyph)
    {
        switch (resolveGlyph(glyph))
        {
        case IconKind::Folder:
            return ImGui::GetColorU32(ImVec4(0.96f, 0.78f, 0.28f, 1.0f));
        case IconKind::File:
            return ImGui::GetColorU32(ImVec4(0.62f, 0.78f, 0.96f, 1.0f));
        case IconKind::Home:
            return ImGui::GetColorU32(ImVec4(0.58f, 0.88f, 0.62f, 1.0f));
        case IconKind::Root:
            return ImGui::GetColorU32(ImVec4(0.86f, 0.64f, 0.92f, 1.0f));
        case IconKind::Current:
            return ImGui::GetColorU32(ImVec4(0.94f, 0.72f, 0.54f, 1.0f));
        case IconKind::Go:
            return ImGui::GetColorU32(ImVec4(0.54f, 0.86f, 0.95f, 1.0f));
        case IconKind::Up:
            return ImGui::GetColorU32(ImVec4(0.84f, 0.82f, 0.96f, 1.0f));
        default:
            return ImGui::GetColorU32(ImGuiCol_Text);
        }
    }

    void drawIconGlyph(ImDrawList *drawList, ImVec2 pos, float size, std::string_view glyph)
    {
        IconKind kind = resolveGlyph(glyph);
        ImU32 fill = iconFillColor(glyph);
        ImU32 stroke = ImGui::GetColorU32(ImGuiCol_Border);
        float rounding = size * 0.18f;

        switch (kind)
        {
        case IconKind::Folder:
        {
            ImVec2 tabMin = ImVec2(pos.x, pos.y + size * 0.1f);
            ImVec2 tabMax = ImVec2(pos.x + size * 0.55f, pos.y + size * 0.38f);
            drawList->AddRectFilled(tabMin, tabMax, fill, rounding);
            ImVec2 bodyMin = ImVec2(pos.x, pos.y + size * 0.28f);
            ImVec2 bodyMax = ImVec2(pos.x + size, pos.y + size);
            drawList->AddRectFilled(bodyMin, bodyMax, fill, rounding);
            drawList->AddRect(bodyMin, bodyMax, stroke, rounding);
            return;
        }
        case IconKind::Go:
        {
            ImVec2 center = ImVec2(pos.x + size * 0.55f, pos.y + size * 0.5f);
            float radius = size * 0.4f;
            drawList->AddCircleFilled(center, radius, fill);
            drawList->AddCircle(center, radius, stroke);
            ImVec2 tip = ImVec2(pos.x + size * 0.85f, pos.y + size * 0.5f);
            ImVec2 left = ImVec2(pos.x + size * 0.52f, pos.y + size * 0.28f);
            ImVec2 right = ImVec2(pos.x + size * 0.52f, pos.y + size * 0.72f);
            drawList->AddTriangleFilled(tip, left, right, ImGui::GetColorU32(ImGuiCol_Text));
            return;
        }
        case IconKind::Up:
        {
            ImVec2 center = ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f);
            float radius = size * 0.42f;
            drawList->AddCircleFilled(center, radius, fill);
            drawList->AddCircle(center, radius, stroke);
            ImVec2 tip = ImVec2(pos.x + size * 0.5f, pos.y + size * 0.18f);
            ImVec2 left = ImVec2(pos.x + size * 0.22f, pos.y + size * 0.6f);
            ImVec2 right = ImVec2(pos.x + size * 0.78f, pos.y + size * 0.6f);
            drawList->AddTriangleFilled(tip, left, right, ImGui::GetColorU32(ImGuiCol_Text));
            return;
        }
        case IconKind::File:
        case IconKind::Home:
        case IconKind::Root:
        case IconKind::Current:
        case IconKind::Unknown:
        default:
            break;
        }

        ImVec2 min = pos;
        ImVec2 max = ImVec2(pos.x + size, pos.y + size);
        drawList->AddRectFilled(min, max, fill, rounding);
        drawList->AddRect(min, max, stroke, rounding);
    }

    bool iconButton(const char *id, std::string_view glyph, const char *tooltip)
    {
        float size = ImGui::GetFrameHeight();
        ImVec2 buttonSize(size, size);
        bool clicked = ImGui::Button(id, buttonSize);
        ImVec2 min = ImGui::GetItemRectMin();

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        float iconSize = size * 0.7f;
        ImVec2 iconPos(min.x + (size - iconSize) * 0.5f, min.y + (size - iconSize) * 0.5f);
        drawIconGlyph(drawList, iconPos, iconSize, glyph);

        if (tooltip && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", tooltip);
        }
        return clicked;
    }
}
