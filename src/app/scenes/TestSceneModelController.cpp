#include "TestSceneModelController.h"

#include <filesystem>

#include "gltf/GltfAsset.h"
#include "render/ModelBuilder.h"
#include "render/ModelRenderer.h"

TestSceneModelController::~TestSceneModelController()
{
    clearModel();
}

bool TestSceneModelController::loadModel(openchordix::render::ModelRenderer &renderer, std::string_view path)
{
    clearModel();
    if (!renderer.isInitialized())
    {
        data_.lastError = "Renderer is not initialized.";
        return false;
    }
    std::filesystem::path modelPath(path);
    if (modelPath.is_relative())
    {
        modelPath = std::filesystem::current_path() / modelPath;
    }

    auto result = openchordix::assets::loadGltfAsset(modelPath);
    if (!result.ok())
    {
        data_.lastError = result.error;
        return false;
    }

    openchordix::render::ModelBuilder builder(renderer);
    openchordix::render::Model model = builder.build(*result.asset);
    if (model.meshes.empty())
    {
        data_.lastError = "Model contains no mesh primitives.";
        return false;
    }

    data_.model = std::make_shared<openchordix::render::Model>(std::move(model));
    data_.assetPath = modelPath.string();
    data_.lastError.clear();
    return true;
}

void TestSceneModelController::clearModel()
{
    data_.model.reset();
    data_.assetPath.clear();
    data_.lastError.clear();
}

std::string TestSceneModelController::status() const
{
    if (!data_.model)
    {
        if (!data_.lastError.empty())
        {
            return std::string("Model error: ") + data_.lastError;
        }
        return std::string("No model loaded.");
    }

    return "Loaded model: " + data_.assetPath +
           " | meshes " + std::to_string(data_.model->meshes.size()) +
           " | materials " + std::to_string(data_.model->materials.size()) +
           " | textures " + std::to_string(data_.model->textures.size());
}
