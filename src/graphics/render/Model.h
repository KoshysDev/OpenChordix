#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

#include <bgfx/bgfx.h>

#include "render/BgfxHandle.h"

namespace openchordix::render
{
    enum class AlphaMode
    {
        Opaque,
        Mask,
        Blend
    };

    struct ModelMaterial
    {
        std::array<float, 4> baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        std::array<float, 3> emissiveFactor{0.0f, 0.0f, 0.0f};
        float emissiveStrength = 1.0f;
        float alphaCutoff = 0.5f;
        float normalScale = 1.0f;
        float occlusionStrength = 1.0f;
        AlphaMode alphaMode = AlphaMode::Opaque;
        bool doubleSided = false;

        uint32_t baseColorTexture = std::numeric_limits<uint32_t>::max();
        uint32_t metallicRoughnessTexture = std::numeric_limits<uint32_t>::max();
        uint32_t normalTexture = std::numeric_limits<uint32_t>::max();
        uint32_t occlusionTexture = std::numeric_limits<uint32_t>::max();
        uint32_t emissiveTexture = std::numeric_limits<uint32_t>::max();
    };

    struct ModelMesh
    {
        BgfxHandle<bgfx::VertexBufferHandle> vertexBuffer{};
        BgfxHandle<bgfx::IndexBufferHandle> indexBuffer{};
        uint32_t indexCount = 0;
        uint16_t materialIndex = 0;
        bool index32 = false;
        std::array<float, 3> boundsMin{0.0f, 0.0f, 0.0f};
        std::array<float, 3> boundsMax{0.0f, 0.0f, 0.0f};

        void destroy();
    };

    struct ModelBounds
    {
        std::array<float, 3> min{0.0f, 0.0f, 0.0f};
        std::array<float, 3> max{0.0f, 0.0f, 0.0f};
        std::array<float, 3> center{0.0f, 0.0f, 0.0f};
        float radius = 0.0f;
        float maxExtent = 0.0f;
    };

    class Model
    {
    public:
        static constexpr uint32_t kInvalidTextureIndex = std::numeric_limits<uint32_t>::max();

        Model() = default;
        ~Model();

        Model(Model &&other) noexcept;
        Model &operator=(Model &&other) noexcept;

        Model(const Model &) = delete;
        Model &operator=(const Model &) = delete;

        void clear();

        std::vector<ModelMesh> meshes;
        std::vector<ModelMaterial> materials;
        std::vector<BgfxHandle<bgfx::TextureHandle>> textures;
        ModelBounds bounds{};
    };
}
