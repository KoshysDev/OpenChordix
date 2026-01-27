#pragma once

#include "gltf/GltfAsset.h"
#include "render/Model.h"

namespace openchordix::render
{
    class ModelRenderer;

    class ModelBuilder
    {
    public:
        explicit ModelBuilder(ModelRenderer &renderer);
        Model build(const openchordix::assets::GltfAsset &asset);

    private:
        ModelRenderer &renderer_;
    };
}
