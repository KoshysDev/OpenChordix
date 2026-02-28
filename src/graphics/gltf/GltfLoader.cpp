#include "gltf/GltfAsset.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>

#include <cstddef>
#include <cstring>
#include <span>
#include <cmath>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
#include <stb/stb_image.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace openchordix::assets
{
    namespace
    {
        constexpr auto kSupportedExtensions =
            fastgltf::Extensions::KHR_mesh_quantization |
            fastgltf::Extensions::KHR_texture_transform |
            fastgltf::Extensions::KHR_materials_emissive_strength;

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
                                                            },
                                                            [&](const fastgltf::sources::Vector &vector) -> std::optional<ImageData>
                                                            {
                                                                if (bufferView.byteOffset + bufferView.byteLength > vector.bytes.size())
                                                                {
                                                                    return std::nullopt;
                                                                }
                                                                std::span<const std::byte> bytes(
                                                                    vector.bytes.data() + bufferView.byteOffset,
                                                                    bufferView.byteLength);
                                                                return decodeImageBytes(bytes);
                                                            },
                                                            [&](const fastgltf::sources::ByteView &viewBytes) -> std::optional<ImageData>
                                                            {
                                                                if (bufferView.byteOffset + bufferView.byteLength > viewBytes.bytes.size())
                                                                {
                                                                    return std::nullopt;
                                                                }
                                                                std::span<const std::byte> bytes(
                                                                    viewBytes.bytes.data() + bufferView.byteOffset,
                                                                    bufferView.byteLength);
                                                                return decodeImageBytes(bytes);
                                                            }},
                                                        buffer.data);
                                  }},
                              image.data);
        }

        std::optional<std::size_t> resolveImageIndex(const fastgltf::Asset &asset, std::size_t textureIndex)
        {
            if (textureIndex >= asset.textures.size())
            {
                return std::nullopt;
            }
            const auto &texture = asset.textures[textureIndex];
            if (texture.imageIndex.has_value())
            {
                return texture.imageIndex.value();
            }
            return std::nullopt;
        }

        openchordix::assets::AlphaMode mapAlphaMode(fastgltf::AlphaMode mode)
        {
            switch (mode)
            {
            case fastgltf::AlphaMode::Mask:
                return openchordix::assets::AlphaMode::Mask;
            case fastgltf::AlphaMode::Blend:
                return openchordix::assets::AlphaMode::Blend;
            case fastgltf::AlphaMode::Opaque:
            default:
                return openchordix::assets::AlphaMode::Opaque;
            }
        }

        void loadMaterials(const fastgltf::Asset &asset, GltfAsset &out)
        {
            out.materials.reserve(asset.materials.size());
            for (const auto &material : asset.materials)
            {
                MaterialData data{};
                const auto &baseColor = material.pbrData.baseColorFactor;
                data.baseColorFactor = {baseColor.x(), baseColor.y(), baseColor.z(), baseColor.w()};
                data.metallicFactor = static_cast<float>(material.pbrData.metallicFactor);
                data.roughnessFactor = static_cast<float>(material.pbrData.roughnessFactor);
                data.emissiveFactor = {material.emissiveFactor.x(), material.emissiveFactor.y(), material.emissiveFactor.z()};
                data.emissiveStrength = static_cast<float>(material.emissiveStrength);
                data.alphaCutoff = static_cast<float>(material.alphaCutoff);
                data.doubleSided = material.doubleSided;
                data.alphaMode = mapAlphaMode(material.alphaMode);

                if (material.pbrData.baseColorTexture.has_value())
                {
                    const auto textureIndex = material.pbrData.baseColorTexture->textureIndex;
                    data.baseColorImageIndex = resolveImageIndex(asset, textureIndex);
                }

                if (material.pbrData.metallicRoughnessTexture.has_value())
                {
                    const auto textureIndex = material.pbrData.metallicRoughnessTexture->textureIndex;
                    data.metallicRoughnessImageIndex = resolveImageIndex(asset, textureIndex);
                }

                if (material.normalTexture.has_value())
                {
                    const auto textureIndex = material.normalTexture->textureIndex;
                    data.normalImageIndex = resolveImageIndex(asset, textureIndex);
                    data.normalScale = static_cast<float>(material.normalTexture->scale);
                }

                if (material.occlusionTexture.has_value())
                {
                    const auto textureIndex = material.occlusionTexture->textureIndex;
                    data.occlusionImageIndex = resolveImageIndex(asset, textureIndex);
                    data.occlusionStrength = static_cast<float>(material.occlusionTexture->strength);
                }

                if (material.emissiveTexture.has_value())
                {
                    const auto textureIndex = material.emissiveTexture->textureIndex;
                    data.emissiveImageIndex = resolveImageIndex(asset, textureIndex);
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
                    bool hasNormals = false;
                    bool hasTexcoords = false;
                    bool hasTangents = false;

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
                        hasNormals = true;
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
                        hasTexcoords = true;
                    }

                    if (const auto *tangentIt = primitive.findAttribute("TANGENT"); tangentIt != primitive.attributes.end())
                    {
                        const auto &tangentAccessor = asset.accessors[tangentIt->accessorIndex];
                        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                            asset,
                            tangentAccessor,
                            [&](fastgltf::math::fvec4 tangent, std::size_t idx)
                            {
                                auto &vertex = primData.vertices[idx];
                                vertex.tangent[0] = tangent.x();
                                vertex.tangent[1] = tangent.y();
                                vertex.tangent[2] = tangent.z();
                                vertex.tangent[3] = tangent.w();
                            });
                        hasTangents = true;
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

                    if (primData.indices.empty() && !primData.vertices.empty())
                    {
                        primData.indices.resize(primData.vertices.size());
                        for (std::size_t i = 0; i < primData.indices.size(); ++i)
                        {
                            primData.indices[i] = static_cast<uint32_t>(i);
                        }
                    }

                    if (!hasNormals && !primData.vertices.empty() && !primData.indices.empty())
                    {
                        for (auto &vertex : primData.vertices)
                        {
                            vertex.normal[0] = 0.0f;
                            vertex.normal[1] = 0.0f;
                            vertex.normal[2] = 0.0f;
                        }

                        for (std::size_t i = 0; i + 2 < primData.indices.size(); i += 3)
                        {
                            const auto &v0 = primData.vertices[primData.indices[i]];
                            const auto &v1 = primData.vertices[primData.indices[i + 1]];
                            const auto &v2 = primData.vertices[primData.indices[i + 2]];

                            float x1 = v1.position[0] - v0.position[0];
                            float y1 = v1.position[1] - v0.position[1];
                            float z1 = v1.position[2] - v0.position[2];
                            float x2 = v2.position[0] - v0.position[0];
                            float y2 = v2.position[1] - v0.position[1];
                            float z2 = v2.position[2] - v0.position[2];

                            float nx = y1 * z2 - z1 * y2;
                            float ny = z1 * x2 - x1 * z2;
                            float nz = x1 * y2 - y1 * x2;

                            auto accumulate = [&](openchordix::assets::Vertex &vertex)
                            {
                                vertex.normal[0] += nx;
                                vertex.normal[1] += ny;
                                vertex.normal[2] += nz;
                            };

                            accumulate(primData.vertices[primData.indices[i]]);
                            accumulate(primData.vertices[primData.indices[i + 1]]);
                            accumulate(primData.vertices[primData.indices[i + 2]]);
                        }

                        for (auto &vertex : primData.vertices)
                        {
                            float len = std::sqrt(vertex.normal[0] * vertex.normal[0] +
                                                  vertex.normal[1] * vertex.normal[1] +
                                                  vertex.normal[2] * vertex.normal[2]);
                            if (len > 0.0f)
                            {
                                vertex.normal[0] /= len;
                                vertex.normal[1] /= len;
                                vertex.normal[2] /= len;
                            }
                            else
                            {
                                vertex.normal[2] = 1.0f;
                            }
                        }
                        hasNormals = true;
                    }

                    if (!hasTangents && hasTexcoords && hasNormals && !primData.vertices.empty() && !primData.indices.empty())
                    {
                        struct TangentAccum
                        {
                            float tx = 0.0f;
                            float ty = 0.0f;
                            float tz = 0.0f;
                            float bx = 0.0f;
                            float by = 0.0f;
                            float bz = 0.0f;
                        };

                        std::vector<TangentAccum> accum(primData.vertices.size());

                        for (std::size_t i = 0; i + 2 < primData.indices.size(); i += 3)
                        {
                            const auto &v0 = primData.vertices[primData.indices[i]];
                            const auto &v1 = primData.vertices[primData.indices[i + 1]];
                            const auto &v2 = primData.vertices[primData.indices[i + 2]];

                            float x1 = v1.position[0] - v0.position[0];
                            float y1 = v1.position[1] - v0.position[1];
                            float z1 = v1.position[2] - v0.position[2];
                            float x2 = v2.position[0] - v0.position[0];
                            float y2 = v2.position[1] - v0.position[1];
                            float z2 = v2.position[2] - v0.position[2];

                            float s1 = v1.uv[0] - v0.uv[0];
                            float t1 = v1.uv[1] - v0.uv[1];
                            float s2 = v2.uv[0] - v0.uv[0];
                            float t2 = v2.uv[1] - v0.uv[1];

                            float denom = (s1 * t2 - s2 * t1);
                            if (std::abs(denom) < 1e-6f)
                            {
                                continue;
                            }
                            float inv = 1.0f / denom;

                            float tx = (t2 * x1 - t1 * x2) * inv;
                            float ty = (t2 * y1 - t1 * y2) * inv;
                            float tz = (t2 * z1 - t1 * z2) * inv;
                            float bx = (s1 * x2 - s2 * x1) * inv;
                            float by = (s1 * y2 - s2 * y1) * inv;
                            float bz = (s1 * z2 - s2 * z1) * inv;

                            auto add = [&](TangentAccum &acc)
                            {
                                acc.tx += tx;
                                acc.ty += ty;
                                acc.tz += tz;
                                acc.bx += bx;
                                acc.by += by;
                                acc.bz += bz;
                            };

                            add(accum[primData.indices[i]]);
                            add(accum[primData.indices[i + 1]]);
                            add(accum[primData.indices[i + 2]]);
                        }

                        for (std::size_t i = 0; i < primData.vertices.size(); ++i)
                        {
                            auto &vertex = primData.vertices[i];
                            auto &acc = accum[i];

                            float nx = vertex.normal[0];
                            float ny = vertex.normal[1];
                            float nz = vertex.normal[2];

                            float tx = acc.tx;
                            float ty = acc.ty;
                            float tz = acc.tz;

                            float dotNT = nx * tx + ny * ty + nz * tz;
                            tx -= nx * dotNT;
                            ty -= ny * dotNT;
                            tz -= nz * dotNT;

                            float len = std::sqrt(tx * tx + ty * ty + tz * tz);
                            if (len > 0.0f)
                            {
                                tx /= len;
                                ty /= len;
                                tz /= len;
                            }
                            else
                            {
                                tx = 1.0f;
                                ty = 0.0f;
                                tz = 0.0f;
                            }

                            float bx = acc.bx;
                            float by = acc.by;
                            float bz = acc.bz;
                            float cx = ny * tz - nz * ty;
                            float cy = nz * tx - nx * tz;
                            float cz = nx * ty - ny * tx;
                            float handedness = (cx * bx + cy * by + cz * bz) < 0.0f ? -1.0f : 1.0f;

                            vertex.tangent[0] = tx;
                            vertex.tangent[1] = ty;
                            vertex.tangent[2] = tz;
                            vertex.tangent[3] = handedness;
                        }
                        hasTangents = true;
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
