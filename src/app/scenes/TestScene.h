#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "AnimatedUI.h"
#include "Scene.h"
#include "ui/FileDialog.h"

class TestSceneModelController;
namespace openchordix::render
{
    class Model;
}

struct TestSceneData
{
    std::shared_ptr<openchordix::render::Model> model;
    std::string assetPath;
    std::string lastError;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[3] = {0.0f, 0.0f, 0.0f};
    float scale = 1.0f;
    bool autoRotate = true;
    float spinSpeed = 24.0f;
    bool useTexture = true;
    bool useMaterialColor = true;
    float tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 1.0f;
    float roughness = 1.0f;
    bool emissiveEnabled = false;
    float emissiveColor[3] = {1.0f, 1.0f, 1.0f};
    float emissiveIntensity = 1.0f;
    float lightDir[3] = {-0.4f, -1.0f, -0.2f};
    float lightColor[3] = {1.0f, 1.0f, 1.0f};
    float lightIntensity = 4.0f;
    float envTopColor[3] = {0.12f, 0.14f, 0.18f};
    float envBottomColor[3] = {0.03f, 0.03f, 0.04f};
    float envIntensity = 1.2f;
    int alphaModeOverride = 0;
    bool glowEnabled = false;
    float glowColor[3] = {0.2f, 0.6f, 1.0f};
    float glowIntensity = 1.0f;
    float glowPower = 2.0f;
};

class TestScene : public Scene
{
public:
    TestScene(AnimatedUI &ui, TestSceneModelController &modelController);

    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    AnimatedUI &ui_;
    TestSceneModelController &modelController_;
    TestSceneData &data_;
    std::array<char, 512> modelPath_{};
    FileDialog modelPicker_;
    bool finished_ = false;
};
