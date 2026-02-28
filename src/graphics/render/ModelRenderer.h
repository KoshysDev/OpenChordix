#pragma once

#include <array>
#include <cstdint>

#include <bgfx/bgfx.h>

#include "gltf/GltfAsset.h"
#include "render/BgfxHandle.h"
#include "render/Model.h"

namespace openchordix::render
{
    struct ModelFrame
    {
        std::array<float, 16> view{};
        std::array<float, 16> proj{};
        std::array<float, 3> cameraPos{0.0f, 0.0f, 0.0f};
        std::array<float, 3> lightDir{-0.4f, -1.0f, -0.2f};
        std::array<float, 3> lightColor{1.0f, 1.0f, 1.0f};
        float lightIntensity = 4.0f;
        std::array<float, 3> envTopColor{0.12f, 0.14f, 0.18f};
        std::array<float, 3> envBottomColor{0.03f, 0.03f, 0.04f};
        float envIntensity = 1.2f;
    };

    struct ModelRenderSettings
    {
        enum class AlphaModeOverride
        {
            Auto,
            Opaque,
            Mask,
            Blend
        };

        bool useBaseColorTexture = true;
        bool useMaterialColor = true;
        std::array<float, 4> tint{1.0f, 1.0f, 1.0f, 1.0f};
        float metallicOverride = 1.0f;
        float roughnessOverride = 1.0f;
        std::array<float, 3> emissiveTint{1.0f, 1.0f, 1.0f};
        float emissiveIntensity = 1.0f;
        float alphaOverride = -1.0f;
        AlphaModeOverride alphaModeOverride = AlphaModeOverride::Auto;
        bool glowEnabled = false;
        std::array<float, 3> glowColor{1.0f, 1.0f, 1.0f};
        float glowIntensity = 1.0f;
        float glowPower = 2.0f;
    };

    class ModelRenderer
    {
    public:
        struct DefaultTextures
        {
            BgfxHandle<bgfx::TextureHandle> baseColor{};
            BgfxHandle<bgfx::TextureHandle> metallicRoughness{};
            BgfxHandle<bgfx::TextureHandle> normal{};
            BgfxHandle<bgfx::TextureHandle> occlusion{};
            BgfxHandle<bgfx::TextureHandle> emissive{};

            void reset()
            {
                baseColor.reset();
                metallicRoughness.reset();
                normal.reset();
                occlusion.reset();
                emissive.reset();
            }

            bool isComplete() const
            {
                return baseColor.isValid() &&
                       metallicRoughness.isValid() &&
                       normal.isValid() &&
                       occlusion.isValid() &&
                       emissive.isValid();
            }
        };

        struct UniformSet
        {
            BgfxHandle<bgfx::UniformHandle> baseColorFactor{};
            BgfxHandle<bgfx::UniformHandle> metallicRoughness{};
            BgfxHandle<bgfx::UniformHandle> emissiveFactor{};
            BgfxHandle<bgfx::UniformHandle> miscParams{};
            BgfxHandle<bgfx::UniformHandle> glowColor{};
            BgfxHandle<bgfx::UniformHandle> glowParams{};
            BgfxHandle<bgfx::UniformHandle> cameraPos{};
            BgfxHandle<bgfx::UniformHandle> lightDir{};
            BgfxHandle<bgfx::UniformHandle> lightColor{};
            BgfxHandle<bgfx::UniformHandle> envTop{};
            BgfxHandle<bgfx::UniformHandle> envBottom{};
            BgfxHandle<bgfx::UniformHandle> envParams{};

            void reset()
            {
                baseColorFactor.reset();
                metallicRoughness.reset();
                emissiveFactor.reset();
                miscParams.reset();
                glowColor.reset();
                glowParams.reset();
                cameraPos.reset();
                lightDir.reset();
                lightColor.reset();
                envTop.reset();
                envBottom.reset();
                envParams.reset();
            }

            bool isComplete() const
            {
                return baseColorFactor.isValid() &&
                       metallicRoughness.isValid() &&
                       emissiveFactor.isValid() &&
                       miscParams.isValid() &&
                       glowColor.isValid() &&
                       glowParams.isValid() &&
                       cameraPos.isValid() &&
                       lightDir.isValid() &&
                       lightColor.isValid() &&
                       envTop.isValid() &&
                       envBottom.isValid() &&
                       envParams.isValid();
            }
        };

        struct SamplerSet
        {
            BgfxHandle<bgfx::UniformHandle> baseColor{};
            BgfxHandle<bgfx::UniformHandle> metallicRoughness{};
            BgfxHandle<bgfx::UniformHandle> normal{};
            BgfxHandle<bgfx::UniformHandle> emissive{};
            BgfxHandle<bgfx::UniformHandle> occlusion{};

            void reset()
            {
                baseColor.reset();
                metallicRoughness.reset();
                normal.reset();
                emissive.reset();
                occlusion.reset();
            }

            bool isComplete() const
            {
                return baseColor.isValid() &&
                       metallicRoughness.isValid() &&
                       normal.isValid() &&
                       emissive.isValid() &&
                       occlusion.isValid();
            }
        };

        bool initialize();
        void shutdown();

        bool isInitialized() const { return initialized_; }
        const bgfx::VertexLayout &vertexLayout() const { return layout_; }
        const DefaultTextures &defaults() const { return defaults_; }

        bgfx::TextureHandle createTextureFromImage(const openchordix::assets::ImageData &image,
                                                   bool srgb) const;

        void renderModel(uint16_t viewId,
                         const Model &model,
                         const float *modelMtx,
                         const ModelFrame &frame,
                         const ModelRenderSettings &settings) const;

    private:
        BgfxHandle<bgfx::ProgramHandle> program_{};
        UniformSet uniforms_{};
        SamplerSet samplers_{};
        bgfx::VertexLayout layout_{};
        DefaultTextures defaults_{};
        bool initialized_ = false;

        static bgfx::TextureHandle createSolidTexture(uint32_t rgba, bool srgb);
    };
}
