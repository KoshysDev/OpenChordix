#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string_view>
#include <type_traits>

#include "EmbeddedAssets.h"

namespace
{
using openchordix::assets::EmbeddedAssetView;

static_assert(std::is_same_v<
              decltype(&openchordix::assets::findEmbeddedAsset),
              std::optional<EmbeddedAssetView> (*)(std::string_view)>);
static_assert(std::is_same_v<
              decltype(&openchordix::assets::listEmbeddedAssets),
              std::span<const EmbeddedAssetView> (*)()>);
}

TEST_CASE("EmbeddedAssetView carries binary payload metadata", "[assets]")
{
    const std::array<unsigned char, 3> bytes{0x01, 0x02, 0x03};
    const EmbeddedAssetView asset{
        bytes.data(),
        bytes.size(),
        "icons/banner.png"};

    CHECK(asset.data == bytes.data());
    CHECK(asset.size == bytes.size());
    CHECK(asset.name == "icons/banner.png");
}
