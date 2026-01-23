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

    struct ImageData
    {
        int width = 0;
        int height = 0;
        std::vector<std::byte> rgba;
    };

    struct MaterialData
    {
        std::array<float, 4> baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        std::optional<std::size_t> baseColorImageIndex;
    };

    struct Vertex
    {
        float position[3]{};
        float normal[3]{};
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
