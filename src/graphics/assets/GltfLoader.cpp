#include "assets/GltfAsset.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>

#include <cstddef>
#include <cstring>
#include <span>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace openchordix::assets
{
    namespace
    {
        constexpr auto kSupportedExtensions =
            fastgltf::Extensions::KHR_mesh_quantization |
            fastgltf::Extensions::KHR_texture_transform;

        constexpr auto kLoadOptions =
            fastgltf::Options::DontRequireValidAssetMember |
            fastgltf::Options::AllowDouble |
            fastgltf::Options::LoadExternalBuffers |
            fastgltf::Options::LoadExternalImages |
            fastgltf::Options::GenerateMeshIndices;

        std::optional<ImageData> decodeImageBytes(std::span<const std::byte> bytes)
        {
            if (bytes.empty())
            {
                return std::nullopt;
            }

            int width = 0;
            int height = 0;
            int channels = 0;
            unsigned char *data = stbi_load_from_memory(
                reinterpret_cast<const stbi_uc *>(bytes.data()),
                static_cast<int>(bytes.size()),
                &width,
                &height,
                &channels,
                4);
            if (!data)
            {
                return std::nullopt;
            }

            ImageData image{};
            image.width = width;
            image.height = height;
            image.rgba.resize(static_cast<std::size_t>(width * height * 4));
            std::memcpy(image.rgba.data(), data, image.rgba.size());
            stbi_image_free(data);
            return image;
        }

        std::optional<ImageData> decodeImageFile(const std::filesystem::path &path)
        {
            int width = 0;
            int height = 0;
            int channels = 0;
            unsigned char *data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
            if (!data)
            {
                return std::nullopt;
            }

            ImageData image{};
            image.width = width;
            image.height = height;
            image.rgba.resize(static_cast<std::size_t>(width * height * 4));
            std::memcpy(image.rgba.data(), data, image.rgba.size());
            stbi_image_free(data);
            return image;
        }

        std::optional<ImageData> loadImageData(const fastgltf::Asset &asset,
                                               const fastgltf::Image &image,
                                               const std::filesystem::path &baseDir)
        {
            return std::visit(fastgltf::visitor{
                                  [](auto &) -> std::optional<ImageData> { return std::nullopt; },
                                  [&](const fastgltf::sources::URI &filePath) -> std::optional<ImageData>
                                  {
                                      if (!filePath.uri.isLocalPath())
                                      {
                                          return std::nullopt;
                                      }
                                      std::filesystem::path resolved = filePath.uri.fspath();
                                      if (resolved.is_relative())
                                      {
                                          resolved = baseDir / resolved;
                                      }
                                      return decodeImageFile(resolved);
                                  },
                                  [&](const fastgltf::sources::Array &array) -> std::optional<ImageData>
                                  {
                                      std::span<const std::byte> bytes(array.bytes.data(), array.bytes.size());
                                      return decodeImageBytes(bytes);
                                  },
                                  [&](const fastgltf::sources::BufferView &view) -> std::optional<ImageData>
                                  {
                                      const auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                                      const auto &buffer = asset.buffers[bufferView.bufferIndex];
                                      return std::visit(fastgltf::visitor{
                                                            [](auto &) -> std::optional<ImageData> { return std::nullopt; },
                                                            [&](const fastgltf::sources::Array &array) -> std::optional<ImageData>
                                                            {
                                                                if (bufferView.byteOffset + bufferView.byteLength > array.bytes.size())
                                                                {
                                                                    return std::nullopt;
                                                                }
                                                                std::span<const std::byte> bytes(
                                                                    array.bytes.data() + bufferView.byteOffset,
                                                                    bufferView.byteLength);
                                                                return decodeImageBytes(bytes);
                                                            }},
                                                        buffer.data);
                                  }},
                              image.data);
        }

        void loadMaterials(const fastgltf::Asset &asset, GltfAsset &out)
        {
            out.materials.reserve(asset.materials.size());
            for (const auto &material : asset.materials)
            {
                MaterialData data{};
                const auto &baseColor = material.pbrData.baseColorFactor;
                data.baseColorFactor = {baseColor.x(), baseColor.y(), baseColor.z(), baseColor.w()};

                if (material.pbrData.baseColorTexture.has_value())
                {
                    const auto textureIndex = material.pbrData.baseColorTexture->textureIndex;
                    if (textureIndex < asset.textures.size())
                    {
                        const auto &texture = asset.textures[textureIndex];
                        if (texture.imageIndex.has_value())
                        {
                            data.baseColorImageIndex = texture.imageIndex.value();
                        }
                    }
                }

                out.materials.emplace_back(std::move(data));
            }
        }

        void loadMeshes(const fastgltf::Asset &asset, GltfAsset &out)
        {
            out.meshes.reserve(asset.meshes.size());
            for (const auto &mesh : asset.meshes)
            {
                MeshData meshData{};
                meshData.name = mesh.name;
                meshData.primitives.reserve(mesh.primitives.size());

                for (const auto &primitive : mesh.primitives)
                {
                    MeshPrimitive primData{};
                    primData.materialIndex = primitive.materialIndex;

                    const auto *positionIt = primitive.findAttribute("POSITION");
                    if (positionIt == primitive.attributes.end())
                    {
                        continue;
                    }

                    const auto &positionAccessor = asset.accessors[positionIt->accessorIndex];
                    primData.vertices.resize(positionAccessor.count);

                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                        asset,
                        positionAccessor,
                        [&](fastgltf::math::fvec3 pos, std::size_t idx)
                        {
                            auto &vertex = primData.vertices[idx];
                            vertex.position[0] = pos.x();
                            vertex.position[1] = pos.y();
                            vertex.position[2] = pos.z();
                        });

                    if (const auto *normalIt = primitive.findAttribute("NORMAL"); normalIt != primitive.attributes.end())
                    {
                        const auto &normalAccessor = asset.accessors[normalIt->accessorIndex];
                        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                            asset,
                            normalAccessor,
                            [&](fastgltf::math::fvec3 normal, std::size_t idx)
                            {
                                auto &vertex = primData.vertices[idx];
                                vertex.normal[0] = normal.x();
                                vertex.normal[1] = normal.y();
                                vertex.normal[2] = normal.z();
                            });
                    }

                    if (const auto *texcoordIt = primitive.findAttribute("TEXCOORD_0"); texcoordIt != primitive.attributes.end())
                    {
                        const auto &texcoordAccessor = asset.accessors[texcoordIt->accessorIndex];
                        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                            asset,
                            texcoordAccessor,
                            [&](fastgltf::math::fvec2 uv, std::size_t idx)
                            {
                                auto &vertex = primData.vertices[idx];
                                vertex.uv[0] = uv.x();
                                vertex.uv[1] = uv.y();
                            });
                    }

                    if (primitive.indicesAccessor.has_value())
                    {
                        const auto &indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
                        primData.indices.resize(indexAccessor.count);

                        if (indexAccessor.componentType == fastgltf::ComponentType::UnsignedByte ||
                            indexAccessor.componentType == fastgltf::ComponentType::UnsignedShort)
                        {
                            std::vector<std::uint16_t> temp(indexAccessor.count);
                            fastgltf::copyFromAccessor<std::uint16_t>(asset, indexAccessor, temp.data());
                            for (std::size_t i = 0; i < temp.size(); ++i)
                            {
                                primData.indices[i] = static_cast<std::uint32_t>(temp[i]);
                            }
                        }
                        else
                        {
                            fastgltf::copyFromAccessor<std::uint32_t>(asset, indexAccessor, primData.indices.data());
                        }
                    }

                    meshData.primitives.emplace_back(std::move(primData));
                }

                out.meshes.emplace_back(std::move(meshData));
            }
        }
    }

    GltfLoadResult loadGltfAsset(const std::filesystem::path &path)
    {
        if (!std::filesystem::exists(path))
        {
            return {std::nullopt, "File not found."};
        }

        fastgltf::Parser parser(kSupportedExtensions);
        auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
        if (!gltfFile)
        {
            return {std::nullopt, std::string(fastgltf::getErrorMessage(gltfFile.error()))};
        }

        auto assetResult = parser.loadGltf(gltfFile.get(), path.parent_path(), kLoadOptions);
        if (assetResult.error() != fastgltf::Error::None)
        {
            return {std::nullopt, std::string(fastgltf::getErrorMessage(assetResult.error()))};
        }

        GltfAsset asset{};
        const auto &parsedAsset = assetResult.get();

        asset.images.reserve(parsedAsset.images.size());
        for (const auto &image : parsedAsset.images)
        {
            auto decoded = loadImageData(parsedAsset, image, path.parent_path());
            if (!decoded)
            {
                return {std::nullopt, "Failed to decode image."};
            }
            asset.images.emplace_back(std::move(*decoded));
        }

        loadMaterials(parsedAsset, asset);
        loadMeshes(parsedAsset, asset);

        return {std::move(asset), {}};
    }

}
