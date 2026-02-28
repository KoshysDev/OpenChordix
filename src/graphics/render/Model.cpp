#include "render/Model.h"

namespace openchordix::render
{
    void ModelMesh::destroy()
    {
        vertexBuffer.reset();
        indexBuffer.reset();
        indexCount = 0;
    }

    Model::~Model()
    {
        clear();
    }

    Model::Model(Model &&other) noexcept
    {
        *this = std::move(other);
    }

    Model &Model::operator=(Model &&other) noexcept
    {
        if (this != &other)
        {
            clear();
            meshes = std::move(other.meshes);
            materials = std::move(other.materials);
            textures = std::move(other.textures);
            bounds = other.bounds;
        }
        return *this;
    }

    void Model::clear()
    {
        meshes.clear();
        materials.clear();
        textures.clear();
        bounds = {};
    }
}
