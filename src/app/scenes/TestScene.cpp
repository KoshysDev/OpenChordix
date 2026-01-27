#include "TestScene.h"

#include <imgui/imgui.h>
#include <bx/math.h>
#include <cmath>
#include <cstring>
#include <filesystem>

#include "TestSceneModelController.h"
#include "render/Model.h"
#include "render/ModelRenderer.h"
#include "render/RenderViewIds.h"

namespace
{
    constexpr float kDegToRad = 0.01745329252f;
}

TestScene::TestScene(AnimatedUI &ui, TestSceneModelController &modelController)
    : ui_(ui),
      modelController_(modelController),
      data_(modelController.data()),
      modelPicker_("test_model_picker", "Model Browser")
{
    if (!data_.assetPath.empty())
    {
        std::strncpy(modelPath_.data(), data_.assetPath.c_str(), modelPath_.size() - 1);
        modelPath_[modelPath_.size() - 1] = '\0';
    }
    modelPicker_.setDirectory(std::filesystem::current_path());
    modelPicker_.setExtensions({".gltf", ".glb"});
}

void TestScene::render(float /*dt*/, const FrameInput & /*input*/, GraphicsContext &gfx, std::atomic<bool> & /*quitFlag*/)
{
    if (data_.model && gfx.modelRenderer().isInitialized())
    {
        const auto &bounds = data_.model->bounds;
        float fitScale = bounds.maxExtent > 0.0f ? 1.8f / bounds.maxExtent : 1.0f;
        float scale = fitScale * data_.scale;

        float autoYaw = data_.autoRotate ? static_cast<float>(ImGui::GetTime()) * data_.spinSpeed : 0.0f;
        float pitch = data_.rotation[0] * kDegToRad;
        float yaw = (data_.rotation[1] + autoYaw) * kDegToRad;
        float roll = data_.rotation[2] * kDegToRad;

        float centerMtx[16];
        float scaleMtx[16];
        float rotMtx[16];
        float transMtx[16];
        float temp[16];
        float temp2[16];
        float modelMtx[16];

        bx::mtxTranslate(centerMtx, -bounds.center[0], -bounds.center[1], -bounds.center[2]);
        bx::mtxScale(scaleMtx, scale, scale, scale);
        bx::mtxRotateXYZ(rotMtx, pitch, yaw, roll);
        bx::mtxTranslate(transMtx, data_.position[0], data_.position[1], data_.position[2]);

        bx::mtxMul(temp, scaleMtx, centerMtx);
        bx::mtxMul(temp2, rotMtx, temp);
        bx::mtxMul(modelMtx, transMtx, temp2);

        float radius = bounds.radius * scale;
        float distance = radius > 0.0f ? radius * 3.0f + 0.5f : 2.5f;
        bx::Vec3 eye = {0.0f, 0.0f, distance};
        bx::Vec3 at = {0.0f, 0.0f, 0.0f};
        bx::Vec3 up = {0.0f, 1.0f, 0.0f};

        float view[16];
        bx::mtxLookAt(view, eye, at, up);

        float proj[16];
        float aspect = gfx.config().height > 0 ? static_cast<float>(gfx.config().width) / static_cast<float>(gfx.config().height) : 1.0f;
        bx::mtxProj(proj, 60.0f, aspect, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

        openchordix::render::ModelFrame frame{};
        std::memcpy(frame.view.data(), view, sizeof(view));
        std::memcpy(frame.proj.data(), proj, sizeof(proj));
        frame.cameraPos = {eye.x, eye.y, eye.z};
        {
            float lx = data_.lightDir[0];
            float ly = data_.lightDir[1];
            float lz = data_.lightDir[2];
            float len = std::sqrt(lx * lx + ly * ly + lz * lz);
            if (len > 1e-4f)
            {
                lx /= len;
                ly /= len;
                lz /= len;
            }
            frame.lightDir = {lx, ly, lz};
            frame.lightColor = {data_.lightColor[0], data_.lightColor[1], data_.lightColor[2]};
            frame.lightIntensity = data_.lightIntensity;
            frame.envTopColor = {data_.envTopColor[0], data_.envTopColor[1], data_.envTopColor[2]};
            frame.envBottomColor = {data_.envBottomColor[0], data_.envBottomColor[1], data_.envBottomColor[2]};
            frame.envIntensity = data_.envIntensity;
        }

        openchordix::render::ModelRenderSettings settings{};
        settings.useBaseColorTexture = data_.useTexture;
        settings.useMaterialColor = data_.useMaterialColor;
        settings.tint = {data_.tint[0], data_.tint[1], data_.tint[2], data_.tint[3]};
        settings.metallicOverride = data_.metallic;
        settings.roughnessOverride = data_.roughness;
        settings.emissiveTint = {data_.emissiveColor[0], data_.emissiveColor[1], data_.emissiveColor[2]};
        settings.emissiveIntensity = data_.emissiveEnabled ? data_.emissiveIntensity : 0.0f;
        settings.alphaModeOverride = static_cast<openchordix::render::ModelRenderSettings::AlphaModeOverride>(data_.alphaModeOverride);
        settings.glowEnabled = data_.glowEnabled;
        settings.glowColor = {data_.glowColor[0], data_.glowColor[1], data_.glowColor[2]};
        settings.glowIntensity = data_.glowIntensity;
        settings.glowPower = data_.glowPower;

        gfx.modelRenderer().renderModel(openchordix::render::kViewIdScene, *data_.model, modelMtx, frame, settings);
    }

    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screen, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("Test Scene", nullptr, flags))
    {
        const float padding = 12.0f;
        ImGui::SetCursorPos(ImVec2(padding, padding));
        if (ui_.button("Back to menu", ImVec2(180.0f, 44.0f)))
        {
            finished_ = true;
        }

        ImVec2 panelPos(padding, padding + 64.0f);
        ImVec2 panelSize(420.0f, screen.y - panelPos.y - padding);
        ImGui::SetCursorPos(panelPos);
        ImGui::BeginChild("Test Scene Controls", panelSize, true, ImGuiWindowFlags_NoMove);

        ImGui::TextUnformatted("Model");
        ImGui::Separator();
        ImGui::InputText("Path", modelPath_.data(), modelPath_.size());
        ImGui::SameLine();
        if (ImGui::Button("Browse..."))
        {
            std::filesystem::path startPath(modelPath_.data());
            std::error_code ec;
            if (!startPath.empty() && std::filesystem::exists(startPath, ec))
            {
                if (std::filesystem::is_directory(startPath, ec))
                {
                    modelPicker_.setDirectory(startPath);
                }
                else
                {
                    auto parent = startPath.parent_path();
                    if (!parent.empty())
                    {
                        modelPicker_.setDirectory(parent);
                    }
                }
            }
            modelPicker_.open();
        }
        if (ImGui::Button("Load"))
        {
            modelController_.loadModel(gfx.modelRenderer(), modelPath_.data());
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            modelController_.clearModel();
        }
        if (!data_.assetPath.empty())
        {
            ImGui::TextWrapped("Loaded: %s", data_.assetPath.c_str());
        }
        if (!data_.lastError.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.55f, 1.0f), "%s", data_.lastError.c_str());
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("Transform");
        ImGui::Separator();
        ImGui::Checkbox("Auto rotate", &data_.autoRotate);
        ImGui::SliderFloat("Spin speed (deg/s)", &data_.spinSpeed, -180.0f, 180.0f, "%.1f");
        ImGui::SliderFloat3("Rotation", data_.rotation, -180.0f, 180.0f, "%.1f");
        ImGui::SliderFloat3("Position", data_.position, -2.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Scale", &data_.scale, 0.1f, 4.0f, "%.2f");
        if (ImGui::Button("Reset transform"))
        {
            data_.position[0] = 0.0f;
            data_.position[1] = 0.0f;
            data_.position[2] = 0.0f;
            data_.rotation[0] = 0.0f;
            data_.rotation[1] = 0.0f;
            data_.rotation[2] = 0.0f;
            data_.scale = 1.0f;
            data_.spinSpeed = 24.0f;
            data_.autoRotate = true;
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("Appearance");
        ImGui::Separator();
        ImGui::Checkbox("Use texture", &data_.useTexture);
        ImGui::Checkbox("Use material color", &data_.useMaterialColor);
        ImGui::ColorEdit4("Tint", data_.tint, ImGuiColorEditFlags_AlphaBar);
        ImGui::SliderFloat("Alpha", &data_.tint[3], 0.0f, 1.0f, "%.2f");

        ImGui::Spacing();
        ImGui::TextUnformatted("Material");
        ImGui::Separator();
        ImGui::SliderFloat("Metallic", &data_.metallic, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Roughness", &data_.roughness, 0.0f, 1.0f, "%.2f");
        const char *alphaModes[] = {"Auto", "Force Opaque", "Force Mask", "Force Blend"};
        ImGui::Combo("Alpha mode", &data_.alphaModeOverride, alphaModes, IM_ARRAYSIZE(alphaModes));

        ImGui::Spacing();
        ImGui::TextUnformatted("Emission");
        ImGui::Separator();
        ImGui::Checkbox("Enable emission", &data_.emissiveEnabled);
        ImGui::BeginDisabled(!data_.emissiveEnabled);
        ImGui::ColorEdit3("Emission color", data_.emissiveColor);
        ImGui::SliderFloat("Emission intensity", &data_.emissiveIntensity, 0.0f, 8.0f, "%.2f");
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::TextUnformatted("Glow");
        ImGui::Separator();
        ImGui::Checkbox("Enable glow", &data_.glowEnabled);
        ImGui::BeginDisabled(!data_.glowEnabled);
        ImGui::ColorEdit3("Glow color", data_.glowColor);
        ImGui::SliderFloat("Glow intensity", &data_.glowIntensity, 0.0f, 8.0f, "%.2f");
        ImGui::SliderFloat("Glow size", &data_.glowPower, 0.5f, 8.0f, "%.2f");
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::TextUnformatted("Lighting");
        ImGui::Separator();
        ImGui::ColorEdit3("Light color", data_.lightColor);
        ImGui::SliderFloat("Light intensity", &data_.lightIntensity, 0.0f, 10.0f, "%.2f");
        ImGui::SliderFloat3("Light direction", data_.lightDir, -1.0f, 1.0f, "%.2f");
        ImGui::ColorEdit3("Sky color", data_.envTopColor);
        ImGui::ColorEdit3("Ground color", data_.envBottomColor);
        ImGui::SliderFloat("Ambient intensity", &data_.envIntensity, 0.0f, 4.0f, "%.2f");
        if (ImGui::Button("Reset lighting"))
        {
            data_.lightDir[0] = -0.4f;
            data_.lightDir[1] = -1.0f;
            data_.lightDir[2] = -0.2f;
            data_.lightColor[0] = 1.0f;
            data_.lightColor[1] = 1.0f;
            data_.lightColor[2] = 1.0f;
            data_.lightIntensity = 4.0f;
            data_.envTopColor[0] = 0.12f;
            data_.envTopColor[1] = 0.14f;
            data_.envTopColor[2] = 0.18f;
            data_.envBottomColor[0] = 0.03f;
            data_.envBottomColor[1] = 0.03f;
            data_.envBottomColor[2] = 0.04f;
            data_.envIntensity = 1.2f;
        }

        ImGui::EndChild();
    }
    ImGui::End();

    if (auto chosen = modelPicker_.draw())
    {
        auto fullPath = chosen->string();
        std::strncpy(modelPath_.data(), fullPath.c_str(), modelPath_.size() - 1);
        modelPath_[modelPath_.size() - 1] = '\0';
        modelController_.loadModel(gfx.modelRenderer(), modelPath_.data());
    }
}
