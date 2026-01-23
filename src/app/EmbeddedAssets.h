#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace openchordix::assets
{
    struct EmbeddedAssetView
    {
        const unsigned char *data;
        std::size_t size;
        std::string_view name;
    };

    std::optional<EmbeddedAssetView> findEmbeddedAsset(std::string_view name);
    std::span<const EmbeddedAssetView> listEmbeddedAssets();
}
