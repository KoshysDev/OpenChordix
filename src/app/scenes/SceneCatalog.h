#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "GraphicsFlow.h"

class SceneCatalog
{
public:
    static std::optional<GraphicsFlow::SceneId> fromName(std::string_view name);
    static std::vector<std::string> names();
};
