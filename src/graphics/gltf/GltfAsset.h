#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openchordix::assets
{
    enum class AlphaMode
    {
        Opaque,
        Mask,
        Blend
    };

    struct ImageData
    {
        int width = 0;
        int height = 0;
        std::vector<std::byte> rgba;
    };

    struct MaterialData
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
        std::optional<std::size_t> baseColorImageIndex;
        std::optional<std::size_t> metallicRoughnessImageIndex;
        std::optional<std::size_t> normalImageIndex;
        std::optional<std::size_t> occlusionImageIndex;
        std::optional<std::size_t> emissiveImageIndex;
    };

    struct Vertex
    {
        float position[3]{};
        float normal[3]{};
        float tangent[4]{1.0f, 0.0f, 0.0f, 1.0f};
        float uv[2]{};
    };

    struct MeshPrimitive
    {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        std::optional<std::size_t> materialIndex;
    };

    struct MeshData
    {
        std::string name;
        std::vector<MeshPrimitive> primitives;
    };

    struct GltfAsset
    {
        std::vector<ImageData> images;
        std::vector<MaterialData> materials;
        std::vector<MeshData> meshes;
    };

    struct GltfLoadResult
    {
        std::optional<GltfAsset> asset;
        std::string error;

        bool ok() const { return asset.has_value(); }
    };

    GltfLoadResult loadGltfAsset(const std::filesystem::path &path);

}
