#include "render/ModelRenderer.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "render/ShaderLoader.h"

namespace openchordix::render
{
    namespace
    {
        constexpr float kAlphaEpsilon = 0.999f;

        bgfx::TextureHandle resolveTexture(const Model &model, uint32_t index, bgfx::TextureHandle fallback)
        {
            if (index == Model::kInvalidTextureIndex || index >= model.textures.size())
            {
                return fallback;
            }
            const auto &handle = model.textures[index];
            if (!handle.isValid())
            {
                return fallback;
            }
            return handle.get();
        }

        struct MaterialParams
        {
            std::array<float, 4> baseColor{};
            std::array<float, 4> metallicRoughness{};
            std::array<float, 4> emissive{};
            std::array<float, 4> miscParams{};
            AlphaMode alphaMode = AlphaMode::Opaque;
            bool doubleSided = false;
        };

        struct GlowParams
        {
            std::array<float, 4> color{};
            std::array<float, 4> params{};
        };

        std::array<float, 4> computeBaseColor(const ModelMaterial &material, const ModelRenderSettings &settings)
        {
            std::array<float, 4> baseColor = settings.tint;
            if (settings.useMaterialColor)
            {
                baseColor[0] *= material.baseColorFactor[0];
                baseColor[1] *= material.baseColorFactor[1];
                baseColor[2] *= material.baseColorFactor[2];
                baseColor[3] *= material.baseColorFactor[3];
            }
            if (settings.alphaOverride >= 0.0f)
            {
                baseColor[3] *= settings.alphaOverride;
            }
            return baseColor;
        }

        std::array<float, 4> computeMetallicRoughness(const ModelMaterial &material, const ModelRenderSettings &settings)
        {
            float metallic = material.metallicFactor;
            float roughness = material.roughnessFactor;

            metallic *= settings.metallicOverride >= 0.0f ? settings.metallicOverride : 1.0f;
            roughness *= settings.roughnessOverride >= 0.0f ? settings.roughnessOverride : 1.0f;

            metallic = std::clamp(metallic, 0.0f, 1.0f);
            roughness = std::clamp(roughness, 0.0f, 1.0f);
            return {metallic, roughness, 0.0f, 0.0f};
        }

        std::array<float, 4> computeEmissive(const ModelMaterial &material, const ModelRenderSettings &settings)
        {
            return {
                material.emissiveFactor[0] * settings.emissiveTint[0],
                material.emissiveFactor[1] * settings.emissiveTint[1],
                material.emissiveFactor[2] * settings.emissiveTint[2],
                material.emissiveStrength * settings.emissiveIntensity};
        }

        AlphaMode resolveAlphaMode(AlphaMode materialMode,
                                   ModelRenderSettings::AlphaModeOverride overrideMode,
                                   float alpha)
        {
            switch (overrideMode)
            {
            case ModelRenderSettings::AlphaModeOverride::Opaque:
                return AlphaMode::Opaque;
            case ModelRenderSettings::AlphaModeOverride::Mask:
                return AlphaMode::Mask;
            case ModelRenderSettings::AlphaModeOverride::Blend:
                return AlphaMode::Blend;
            case ModelRenderSettings::AlphaModeOverride::Auto:
            default:
                break;
            }

            if (alpha < kAlphaEpsilon)
            {
                return AlphaMode::Blend;
            }
            return materialMode;
        }

        bool resolveDoubleSided(bool materialDoubleSided, float alpha)
        {
            return alpha < kAlphaEpsilon ? true : materialDoubleSided;
        }

        MaterialParams buildMaterialParams(const ModelMaterial &material,
                                            const ModelRenderSettings &settings)
        {
            MaterialParams params{};
            params.baseColor = computeBaseColor(material, settings);
            params.metallicRoughness = computeMetallicRoughness(material, settings);
            params.emissive = computeEmissive(material, settings);
            params.alphaMode = resolveAlphaMode(material.alphaMode, settings.alphaModeOverride, params.baseColor[3]);
            params.doubleSided = resolveDoubleSided(material.doubleSided, params.baseColor[3]);

            float alphaCutoff = params.alphaMode == AlphaMode::Mask ? material.alphaCutoff : -1.0f;
            params.miscParams = {alphaCutoff,
                                 settings.useBaseColorTexture ? 1.0f : 0.0f,
                                 material.occlusionStrength,
                                 material.normalScale};
            return params;
        }

        GlowParams buildGlowParams(const ModelRenderSettings &settings)
        {
            GlowParams glow{};
            glow.color = {settings.glowColor[0], settings.glowColor[1], settings.glowColor[2], 1.0f};
            glow.params = {settings.glowEnabled ? settings.glowIntensity : 0.0f,
                           settings.glowPower,
                           0.0f,
                           0.0f};
            return glow;
        }

        uint64_t computeState(AlphaMode alphaMode, bool doubleSided)
        {
            uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA;
            if (alphaMode != AlphaMode::Blend)
            {
                state |= BGFX_STATE_WRITE_Z;
            }
            if (!doubleSided)
            {
                state |= BGFX_STATE_CULL_CW;
            }
            if (alphaMode == AlphaMode::Blend)
            {
                state |= BGFX_STATE_BLEND_ALPHA;
            }
            return state;
        }
    }

    bool ModelRenderer::initialize()
    {
        if (initialized_)
        {
            return true;
        }

        layout_.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        BgfxHandle<bgfx::ProgramHandle> program(loadEmbeddedProgram("vs_standard.bin", "fs_standard.bin"));
        if (!program.isValid())
        {
            std::cerr << "ModelRenderer: Failed to load standard shader program." << std::endl;
            return false;
        }

        UniformSet uniforms{};
        uniforms.baseColorFactor.reset(bgfx::createUniform("u_baseColorFactor", bgfx::UniformType::Vec4));
        uniforms.metallicRoughness.reset(bgfx::createUniform("u_metallicRoughness", bgfx::UniformType::Vec4));
        uniforms.emissiveFactor.reset(bgfx::createUniform("u_emissiveFactor", bgfx::UniformType::Vec4));
        uniforms.miscParams.reset(bgfx::createUniform("u_miscParams", bgfx::UniformType::Vec4));
        uniforms.glowColor.reset(bgfx::createUniform("u_glowColor", bgfx::UniformType::Vec4));
        uniforms.glowParams.reset(bgfx::createUniform("u_glowParams", bgfx::UniformType::Vec4));
        uniforms.cameraPos.reset(bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4));
        uniforms.lightDir.reset(bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4));
        uniforms.lightColor.reset(bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4));
        uniforms.envTop.reset(bgfx::createUniform("u_envTop", bgfx::UniformType::Vec4));
        uniforms.envBottom.reset(bgfx::createUniform("u_envBottom", bgfx::UniformType::Vec4));
        uniforms.envParams.reset(bgfx::createUniform("u_envParams", bgfx::UniformType::Vec4));

        SamplerSet samplers{};
        samplers.baseColor.reset(bgfx::createUniform("s_baseColor", bgfx::UniformType::Sampler));
        samplers.metallicRoughness.reset(bgfx::createUniform("s_metallicRoughness", bgfx::UniformType::Sampler));
        samplers.normal.reset(bgfx::createUniform("s_normal", bgfx::UniformType::Sampler));
        samplers.emissive.reset(bgfx::createUniform("s_emissive", bgfx::UniformType::Sampler));
        samplers.occlusion.reset(bgfx::createUniform("s_occlusion", bgfx::UniformType::Sampler));

        DefaultTextures defaults{};
        defaults.baseColor.reset(createSolidTexture(0xffffffff, true));
        defaults.metallicRoughness.reset(createSolidTexture(0xffffffff, false));
        defaults.normal.reset(createSolidTexture(0x8080ffff, false));
        defaults.occlusion.reset(createSolidTexture(0xffffffff, false));
        defaults.emissive.reset(createSolidTexture(0xffffffff, true));

        if (!uniforms.isComplete() || !samplers.isComplete() || !defaults.isComplete())
        {
            return false;
        }

        program_ = std::move(program);
        uniforms_ = std::move(uniforms);
        samplers_ = std::move(samplers);
        defaults_ = std::move(defaults);

        initialized_ = true;
        return true;
    }

    void ModelRenderer::shutdown()
    {
        defaults_.reset();
        samplers_.reset();
        uniforms_.reset();
        program_.reset();
        initialized_ = false;
    }

    bgfx::TextureHandle ModelRenderer::createTextureFromImage(const openchordix::assets::ImageData &image,
                                                              bool srgb) const
    {
        if (image.width <= 0 || image.height <= 0 || image.rgba.empty())
        {
            return BGFX_INVALID_HANDLE;
        }

        uint64_t flags = 0;
        if (srgb)
        {
            flags |= BGFX_TEXTURE_SRGB;
        }

        const bgfx::Memory *mem = bgfx::copy(image.rgba.data(), static_cast<uint32_t>(image.rgba.size()));
        return bgfx::createTexture2D(static_cast<uint16_t>(image.width),
                                     static_cast<uint16_t>(image.height),
                                     false,
                                     1,
                                     bgfx::TextureFormat::RGBA8,
                                     flags,
                                     mem);
    }

    void ModelRenderer::renderModel(uint16_t viewId,
                                    const Model &model,
                                    const float *modelMtx,
                                    const ModelFrame &frame,
                                    const ModelRenderSettings &settings) const
    {
        if (!initialized_ || !program_.isValid() || model.meshes.empty() || model.materials.empty())
        {
            return;
        }

        bgfx::setViewTransform(viewId, frame.view.data(), frame.proj.data());

        float cameraPos[4] = {frame.cameraPos[0], frame.cameraPos[1], frame.cameraPos[2], 1.0f};
        bgfx::setUniform(uniforms_.cameraPos.get(), cameraPos);

        float lightDir[4] = {frame.lightDir[0], frame.lightDir[1], frame.lightDir[2], 0.0f};
        bgfx::setUniform(uniforms_.lightDir.get(), lightDir);

        float lightColor[4] = {frame.lightColor[0], frame.lightColor[1], frame.lightColor[2], frame.lightIntensity};
        bgfx::setUniform(uniforms_.lightColor.get(), lightColor);

        float envTop[4] = {frame.envTopColor[0], frame.envTopColor[1], frame.envTopColor[2], 1.0f};
        float envBottom[4] = {frame.envBottomColor[0], frame.envBottomColor[1], frame.envBottomColor[2], 1.0f};
        float envParams[4] = {frame.envIntensity, 0.0f, 0.0f, 0.0f};
        bgfx::setUniform(uniforms_.envTop.get(), envTop);
        bgfx::setUniform(uniforms_.envBottom.get(), envBottom);
        bgfx::setUniform(uniforms_.envParams.get(), envParams);

        GlowParams glow = buildGlowParams(settings);

        for (const auto &mesh : model.meshes)
        {
            if (!mesh.vertexBuffer.isValid() || !mesh.indexBuffer.isValid() || mesh.indexCount == 0)
            {
                continue;
            }

            const ModelMaterial &material = mesh.materialIndex < model.materials.size()
                                                ? model.materials[mesh.materialIndex]
                                                : model.materials.front();

            MaterialParams params = buildMaterialParams(material, settings);

            bgfx::setTransform(modelMtx);
            bgfx::setVertexBuffer(0, mesh.vertexBuffer.get());
            bgfx::setIndexBuffer(mesh.indexBuffer.get(), 0, mesh.indexCount);

            bgfx::setUniform(uniforms_.baseColorFactor.get(), params.baseColor.data());
            bgfx::setUniform(uniforms_.metallicRoughness.get(), params.metallicRoughness.data());
            bgfx::setUniform(uniforms_.emissiveFactor.get(), params.emissive.data());
            bgfx::setUniform(uniforms_.miscParams.get(), params.miscParams.data());
            bgfx::setUniform(uniforms_.glowColor.get(), glow.color.data());
            bgfx::setUniform(uniforms_.glowParams.get(), glow.params.data());

            bgfx::TextureHandle baseColorTex = resolveTexture(model, material.baseColorTexture, defaults_.baseColor.get());
            bgfx::TextureHandle metallicRoughnessTex = resolveTexture(model, material.metallicRoughnessTexture, defaults_.metallicRoughness.get());
            bgfx::TextureHandle normalTex = resolveTexture(model, material.normalTexture, defaults_.normal.get());
            bgfx::TextureHandle emissiveTex = resolveTexture(model, material.emissiveTexture, defaults_.emissive.get());
            bgfx::TextureHandle occlusionTex = resolveTexture(model, material.occlusionTexture, defaults_.occlusion.get());

            bgfx::setTexture(0, samplers_.baseColor.get(), baseColorTex);
            bgfx::setTexture(1, samplers_.metallicRoughness.get(), metallicRoughnessTex);
            bgfx::setTexture(2, samplers_.normal.get(), normalTex);
            bgfx::setTexture(3, samplers_.emissive.get(), emissiveTex);
            bgfx::setTexture(4, samplers_.occlusion.get(), occlusionTex);

            uint64_t state = computeState(params.alphaMode, params.doubleSided);

            bgfx::setState(state);
            bgfx::submit(viewId, program_.get());
        }
    }

    bgfx::TextureHandle ModelRenderer::createSolidTexture(uint32_t rgba, bool srgb)
    {
        uint8_t pixel[4] = {
            static_cast<uint8_t>((rgba >> 24) & 0xff),
            static_cast<uint8_t>((rgba >> 16) & 0xff),
            static_cast<uint8_t>((rgba >> 8) & 0xff),
            static_cast<uint8_t>(rgba & 0xff)};

        uint64_t flags = 0;
        if (srgb)
        {
            flags |= BGFX_TEXTURE_SRGB;
        }

        const bgfx::Memory *mem = bgfx::copy(pixel, sizeof(pixel));
        return bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, flags, mem);
    }
}
