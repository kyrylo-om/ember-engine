#include <catch_amalgamated.hpp>

import ember.assets;
import std;

using ember::assets::AssetCatalog;

namespace
{

constexpr std::array girl_png{std::byte{1}, std::byte{2}, std::byte{3}};
constexpr std::array other_png{std::byte{4}, std::byte{5}};
constexpr std::array jump_wav{std::byte{6}};

} // namespace

TEST_CASE("an empty catalog contains no files")
{
    const AssetCatalog catalog;

    CHECK(catalog.size() == 0);
    CHECK_FALSE(catalog.contains("girl.png"));
    CHECK_FALSE(catalog.find("girl.png").has_value());
}

TEST_CASE("a file is found with the same bytes, which are not copied")
{
    AssetCatalog catalog;
    catalog.add("girl.png", girl_png);

    const auto found = catalog.find("girl.png");

    REQUIRE(found.has_value());
    CHECK(found->data() == girl_png.data());
    CHECK(found->size() == girl_png.size());
    CHECK(catalog.contains("girl.png"));
}

TEST_CASE("adding a file under a name that is taken replaces it")
{
    AssetCatalog catalog;
    catalog.add("girl.png", girl_png);

    catalog.add("girl.png", other_png);

    REQUIRE(catalog.find("girl.png").has_value());
    CHECK(catalog.find("girl.png")->data() == other_png.data());
    CHECK(catalog.size() == 1);
}

TEST_CASE("names are compared exactly")
{
    AssetCatalog catalog;
    catalog.add("girl.png", girl_png);

    CHECK_FALSE(catalog.contains("Girl.png"));
    CHECK_FALSE(catalog.contains("girl"));
    CHECK_FALSE(catalog.contains("girl.png "));
}

TEST_CASE("every file of a catalog is counted and found separately")
{
    AssetCatalog catalog;
    catalog.add("girl.png", girl_png);
    catalog.add("other.png", other_png);
    catalog.add("jump.wav", jump_wav);

    CHECK(catalog.size() == 3);
    CHECK(catalog.find("girl.png")->data() == girl_png.data());
    CHECK(catalog.find("other.png")->data() == other_png.data());
    CHECK(catalog.find("jump.wav")->data() == jump_wav.data());
}
