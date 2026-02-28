#pragma once

#include <string>
#include <string_view>

#include "TestScene.h"

namespace openchordix::render
{
    class ModelRenderer;
}

class TestSceneModelController
{
public:
    TestSceneModelController() = default;
    ~TestSceneModelController();

    TestSceneData &data() { return data_; }
    const TestSceneData &data() const { return data_; }

    bool loadModel(openchordix::render::ModelRenderer &renderer, std::string_view path);
    void clearModel();
    std::string status() const;

private:
    TestSceneData data_{};
};
