#include "render/ModelBuilder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <numeric>
#include <ranges>
#include <utility>
#include <vector>

#include "render/BgfxHandle.h"
#include "render/ModelRenderer.h"

namespace openchordix::render
{
    namespace
    {
        AlphaMode mapAlphaMode(openchordix::assets::AlphaMode mode)
        {
            switch (mode)
            {
            case openchordix::assets::AlphaMode::Mask:
                return AlphaMode::Mask;
            case openchordix::assets::AlphaMode::Blend:
                return AlphaMode::Blend;
            case openchordix::assets::AlphaMode::Opaque:
            default:
                return AlphaMode::Opaque;
            }
        }

        struct ImageTextures
        {
            uint32_t srgb = Model::kInvalidTextureIndex;
            uint32_t linear = Model::kInvalidTextureIndex;
        };
    }

    ModelBuilder::ModelBuilder(ModelRenderer &renderer)
        : renderer_(renderer)
    {
    }

    Model ModelBuilder::build(const openchordix::assets::GltfAsset &asset)
    {
        Model model;
        model.materials.reserve(asset.materials.size());

        std::vector<ImageTextures> imageTextures(asset.images.size());

        auto getTextureIndex = [&](std::size_t imageIndex, bool srgb) -> uint32_t
        {
            if (imageIndex >= asset.images.size())
            {
                return Model::kInvalidTextureIndex;
            }
            ImageTextures &entry = imageTextures[imageIndex];
            uint32_t &slot = srgb ? entry.srgb : entry.linear;
            if (slot != Model::kInvalidTextureIndex)
            {
                return slot;
            }

            bgfx::TextureHandle handle = renderer_.createTextureFromImage(asset.images[imageIndex], srgb);
            if (!bgfx::isValid(handle))
            {
                return Model::kInvalidTextureIndex;
            }

            model.textures.emplace_back(handle);
            slot = static_cast<uint32_t>(model.textures.size() - 1);
            return slot;
        };

        auto buildIndices = [](const openchordix::assets::MeshPrimitive &prim)
        {
            std::vector<uint32_t> indices = prim.indices;
            if (indices.empty())
            {
                indices.resize(prim.vertices.size());
                std::iota(indices.begin(), indices.end(), 0u);
            }
            return indices;
        };

        auto createVertexBuffer = [&](const std::vector<openchordix::assets::Vertex> &vertices)
        {
            const bgfx::Memory *vbm = bgfx::copy(vertices.data(),
                                                 static_cast<uint32_t>(vertices.size() * sizeof(openchordix::assets::Vertex)));
            return BgfxHandle<bgfx::VertexBufferHandle>(bgfx::createVertexBuffer(vbm, renderer_.vertexLayout()));
        };

        struct IndexBufferResult
        {
            BgfxHandle<bgfx::IndexBufferHandle> handle{};
            bool index32 = false;
        };

        auto createIndexBuffer = [](const std::vector<uint32_t> &indices)
        {
            IndexBufferResult result{};
            if (indices.empty())
            {
                return result;
            }

            uint32_t maxIndex = *std::max_element(indices.begin(), indices.end());
            result.index32 = maxIndex > std::numeric_limits<uint16_t>::max();

            if (result.index32)
            {
                const bgfx::Memory *ibm = bgfx::copy(indices.data(),
                                                     static_cast<uint32_t>(indices.size() * sizeof(uint32_t)));
                result.handle.reset(bgfx::createIndexBuffer(ibm, BGFX_BUFFER_INDEX32));
                return result;
            }

            std::vector<uint16_t> indices16(indices.size());
            std::transform(indices.begin(), indices.end(), indices16.begin(),
                           [](uint32_t value) { return static_cast<uint16_t>(value); });
            const bgfx::Memory *ibm = bgfx::copy(indices16.data(),
                                                 static_cast<uint32_t>(indices16.size() * sizeof(uint16_t)));
            result.handle.reset(bgfx::createIndexBuffer(ibm));
            return result;
        };

        auto computeBounds = [](const std::vector<openchordix::assets::Vertex> &vertices)
        {
            std::array<float, 3> min = {vertices.front().position[0], vertices.front().position[1], vertices.front().position[2]};
            std::array<float, 3> max = min;
            for (const auto &vertex : vertices)
            {
                min[0] = std::min(min[0], vertex.position[0]);
                min[1] = std::min(min[1], vertex.position[1]);
                min[2] = std::min(min[2], vertex.position[2]);
                max[0] = std::max(max[0], vertex.position[0]);
                max[1] = std::max(max[1], vertex.position[1]);
                max[2] = std::max(max[2], vertex.position[2]);
            }
            return std::pair{min, max};
        };

        auto mergeBounds = [](ModelBounds &bounds,
                              const std::array<float, 3> &min,
                              const std::array<float, 3> &max,
                              bool &boundsInit)
        {
            if (!boundsInit)
            {
                bounds.min = min;
                bounds.max = max;
                boundsInit = true;
                return;
            }

            bounds.min[0] = std::min(bounds.min[0], min[0]);
            bounds.min[1] = std::min(bounds.min[1], min[1]);
            bounds.min[2] = std::min(bounds.min[2], min[2]);
            bounds.max[0] = std::max(bounds.max[0], max[0]);
            bounds.max[1] = std::max(bounds.max[1], max[1]);
            bounds.max[2] = std::max(bounds.max[2], max[2]);
        };

        auto applyTextureIndex = [&](uint32_t &target,
                                     const std::optional<std::size_t> &imageIndex,
                                     bool srgb)
        {
            if (imageIndex)
            {
                target = getTextureIndex(*imageIndex, srgb);
            }
        };

        auto buildMaterial = [&](const openchordix::assets::MaterialData &material)
        {
            ModelMaterial out{
                .baseColorFactor = material.baseColorFactor,
                .metallicFactor = material.metallicFactor,
                .roughnessFactor = material.roughnessFactor,
                .emissiveFactor = material.emissiveFactor,
                .emissiveStrength = material.emissiveStrength,
                .alphaCutoff = material.alphaCutoff,
                .normalScale = material.normalScale,
                .occlusionStrength = material.occlusionStrength,
                .alphaMode = mapAlphaMode(material.alphaMode),
                .doubleSided = material.doubleSided};

            applyTextureIndex(out.baseColorTexture, material.baseColorImageIndex, true);
            applyTextureIndex(out.metallicRoughnessTexture, material.metallicRoughnessImageIndex, false);
            applyTextureIndex(out.normalTexture, material.normalImageIndex, false);
            applyTextureIndex(out.occlusionTexture, material.occlusionImageIndex, false);
            applyTextureIndex(out.emissiveTexture, material.emissiveImageIndex, true);

            return out;
        };

        std::ranges::transform(asset.materials, std::back_inserter(model.materials), buildMaterial);

        if (model.materials.empty())
        {
            model.materials.push_back(ModelMaterial{});
        }

        bool boundsInit = false;

        for (const auto &mesh : asset.meshes)
        {
            for (const auto &prim : mesh.primitives)
            {
                if (prim.vertices.empty())
                {
                    continue;
                }

                std::vector<uint32_t> indices = buildIndices(prim);
                auto vbh = createVertexBuffer(prim.vertices);
                auto indexBuffer = createIndexBuffer(indices);

                if (!vbh.isValid() || !indexBuffer.handle.isValid())
                {
                    continue;
                }

                ModelMesh outMesh{};
                outMesh.vertexBuffer = std::move(vbh);
                outMesh.indexBuffer = std::move(indexBuffer.handle);
                outMesh.indexCount = static_cast<uint32_t>(indices.size());
                outMesh.index32 = indexBuffer.index32;
                if (prim.materialIndex && *prim.materialIndex < model.materials.size())
                {
                    outMesh.materialIndex = static_cast<uint16_t>(*prim.materialIndex);
                }

                auto [minBounds, maxBounds] = computeBounds(prim.vertices);

                outMesh.boundsMin = minBounds;
                outMesh.boundsMax = maxBounds;
                model.meshes.push_back(std::move(outMesh));

                mergeBounds(model.bounds, minBounds, maxBounds, boundsInit);
            }
        }

        if (boundsInit)
        {
            model.bounds.center = {
                0.5f * (model.bounds.min[0] + model.bounds.max[0]),
                0.5f * (model.bounds.min[1] + model.bounds.max[1]),
                0.5f * (model.bounds.min[2] + model.bounds.max[2])};

            float dx = model.bounds.max[0] - model.bounds.min[0];
            float dy = model.bounds.max[1] - model.bounds.min[1];
            float dz = model.bounds.max[2] - model.bounds.min[2];
            model.bounds.maxExtent = std::max(dx, std::max(dy, dz));
            model.bounds.radius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        return model;
    }
}
