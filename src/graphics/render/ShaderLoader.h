#pragma once

#include <bgfx/bgfx.h>
#include <string>
#include <string_view>

namespace openchordix::render
{
    std::string shaderBackendTag(bgfx::RendererType::Enum type);
    bgfx::ProgramHandle loadEmbeddedProgram(std::string_view vsName, std::string_view fsName);
}
