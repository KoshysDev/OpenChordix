#include "ShaderLoader.h"

#include <iostream>
#include <string>
#include <vector>

#include "EmbeddedAssets.h"
#include "render/BgfxHandle.h"

namespace openchordix::render
{
    namespace
    {
        std::vector<std::string_view> shaderBackendCandidates(bgfx::RendererType::Enum type)
        {
            switch (type)
            {
            case bgfx::RendererType::Vulkan:
                return {"spirv", "glsl"};
            case bgfx::RendererType::OpenGL:
            case bgfx::RendererType::OpenGLES:
                return {"glsl", "spirv"};
            case bgfx::RendererType::Metal:
                return {"metal", "spirv", "glsl"};
            case bgfx::RendererType::Direct3D11:
                return {"dxbc", "dx11", "dx12"};
            case bgfx::RendererType::Direct3D12:
                return {"dxbc", "dx12", "dx11"};
            default:
                return {"glsl", "spirv", "dxbc"};
            }
        }

        bool hasEmbeddedShader(std::string_view path)
        {
            return openchordix::assets::findEmbeddedAsset(path).has_value();
        }

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
            return "dxbc";
        case bgfx::RendererType::Direct3D12:
            return "dxbc";
        default:
            return "glsl";
        }
    }

    bgfx::ProgramHandle loadEmbeddedProgram(std::string_view vsName, std::string_view fsName)
    {
        const auto rendererType = bgfx::getRendererType();
        const auto backends = shaderBackendCandidates(rendererType);

        for (std::string_view backend : backends)
        {
            const std::string vsPath = "shaders/" + std::string(backend) + "/" + std::string(vsName);
            const std::string fsPath = "shaders/" + std::string(backend) + "/" + std::string(fsName);

            // Avoid noisy logs for fallback backends that were not packaged.
            if (!hasEmbeddedShader(vsPath) || !hasEmbeddedShader(fsPath))
            {
                continue;
            }

            bgfx::ShaderHandle vsHandle = loadEmbeddedShader(vsPath);
            if (!bgfx::isValid(vsHandle))
            {
                continue;
            }
            bgfx::ShaderHandle fsHandle = loadEmbeddedShader(fsPath);
            if (!bgfx::isValid(fsHandle))
            {
                destroyIfValid(vsHandle);
                continue;
            }

            auto program = bgfx::createProgram(vsHandle, fsHandle, true);
            if (bgfx::isValid(program))
            {
                return program;
            }
        }

        std::cerr << "ShaderLoader: Missing embedded shader pair for renderer "
                  << bgfx::getRendererName(rendererType) << ". Tried backends:";
        for (std::string_view backend : backends)
        {
            std::cerr << " " << backend;
        }
        std::cerr << std::endl;
        return BGFX_INVALID_HANDLE;
    }
}
