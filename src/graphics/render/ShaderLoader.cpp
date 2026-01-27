#include "ShaderLoader.h"

#include <iostream>
#include <string>

#include "EmbeddedAssets.h"
#include "render/BgfxHandle.h"

namespace openchordix::render
{
    namespace
    {
        bgfx::ShaderHandle loadEmbeddedShader(std::string_view path)
        {
            auto asset = openchordix::assets::findEmbeddedAsset(path);
            if (!asset)
            {
                std::cerr << "ShaderLoader: Missing embedded shader " << path << std::endl;
                return BGFX_INVALID_HANDLE;
            }

            const bgfx::Memory *mem = bgfx::copy(asset->data, static_cast<uint32_t>(asset->size));
            auto handle = bgfx::createShader(mem);
            if (bgfx::isValid(handle))
            {
                bgfx::setName(handle, std::string(path).c_str());
            }
            return handle;
        }
    }

    std::string shaderBackendTag(bgfx::RendererType::Enum type)
    {
        switch (type)
        {
        case bgfx::RendererType::Vulkan:
            return "spirv";
        case bgfx::RendererType::OpenGL:
        case bgfx::RendererType::OpenGLES:
            return "glsl";
        case bgfx::RendererType::Metal:
            return "metal";
        case bgfx::RendererType::Direct3D11:
            return "dx11";
        case bgfx::RendererType::Direct3D12:
            return "dx12";
        default:
            return "glsl";
        }
    }

    bgfx::ProgramHandle loadEmbeddedProgram(std::string_view vsName, std::string_view fsName)
    {
        const std::string backend = shaderBackendTag(bgfx::getRendererType());
        const std::string vsPath = "shaders/" + backend + "/" + std::string(vsName);
        const std::string fsPath = "shaders/" + backend + "/" + std::string(fsName);

        bgfx::ShaderHandle vsHandle = loadEmbeddedShader(vsPath);
        if (!bgfx::isValid(vsHandle))
        {
            return BGFX_INVALID_HANDLE;
        }
        bgfx::ShaderHandle fsHandle = loadEmbeddedShader(fsPath);
        if (!bgfx::isValid(fsHandle))
        {
            destroyIfValid(vsHandle);
            return BGFX_INVALID_HANDLE;
        }

        return bgfx::createProgram(vsHandle, fsHandle, true);
    }
}
