#pragma once

#include <memory>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>

#include "AnimatedUI.h"
#include "Scene.h"
#include "assets/GltfAsset.h"

struct TestSceneData
{
    std::shared_ptr<openchordix::assets::GltfAsset> asset;
    std::string assetPath;
    std::string lastError;
    std::vector<bgfx::TextureHandle> imageTextures;
};

class TestScene : public Scene
{
public:
    TestScene(AnimatedUI &ui, const TestSceneData &data);

    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    AnimatedUI &ui_;
    const TestSceneData &data_;
    bool finished_ = false;
};
