#include <catch_amalgamated.hpp>

import ember.assets;
import ember.common;
import ember.core;
import ember.platform;
import std;

using ember::Handle;
using ember::Texture;
using ember::Vec2;
using ember::assets::AssetCatalog;
using ember::assets::AssetError;
using ember::assets::AssetManager;
using ember::assets::AssetsPlugin;
using ember::core::Application;
using ember::platform::Graphics;
using ember::platform::PlatformPlugin;

namespace
{

// Embedded like a game embeds its assets. A bitmap of 2 x 1 pixels and 0.05 s of silence.
constexpr unsigned char pixel_bmp[] = {
#embed "data/pixel.bmp"
};
constexpr unsigned char silence_wav[] = {
#embed "data/silence.wav"
};

// Data that is neither an image, a sound nor a font.
constexpr std::array<std::byte, 64> garbage = []
{
    std::array<std::byte, 64> bytes{};
    bytes.fill(std::byte{7});
    return bytes;
}();

void add_test_files(AssetCatalog& catalog)
{
    catalog.add("pixel.bmp", std::as_bytes(std::span{pixel_bmp}));
    catalog.add("silence.wav", std::as_bytes(std::span{silence_wav}));
    catalog.add("broken.png", garbage);
    catalog.add("broken.ttf", garbage);
}

// Adds the platform plugin; false if this machine cannot open a window.
bool add_platform(Application& app, bool is_audio_enabled)
{
    app.add_plugin(PlatformPlugin{
        .settings = {.window = {.title = "Ember assets test", .width = 320, .height = 240},
                     .is_audio_enabled = is_audio_enabled}});
    return app.world().try_resource<ember::platform::Window>() != nullptr;
}

} // namespace

TEST_CASE("the plugin inserts an empty catalog and a manager that has loaded nothing")
{
    Application app;
    app.add_plugin(AssetsPlugin{});

    REQUIRE(app.world().try_resource<AssetCatalog>() != nullptr);
    REQUIRE(app.world().try_resource<AssetManager>() != nullptr);
    CHECK(app.world().resource<AssetCatalog>().size() == 0);
    CHECK(app.world().resource<AssetManager>().loaded_count() == 0);
}

TEST_CASE("a name that is not in the catalog is not found")
{
    const AssetCatalog catalog;
    AssetManager manager{catalog, nullptr, nullptr};

    CHECK(manager.load_texture("missing.png").error() == AssetError::NotFound);
    CHECK(manager.load_sound("missing.wav").error() == AssetError::NotFound);
    CHECK(manager.load_music("missing.wav").error() == AssetError::NotFound);
    CHECK(manager.load_font("missing.ttf", 16).error() == AssetError::NotFound);
}

TEST_CASE("without the platform services nothing can be loaded")
{
    AssetCatalog catalog;
    add_test_files(catalog);
    AssetManager manager{catalog, nullptr, nullptr};

    CHECK(manager.load_texture("pixel.bmp").error() == AssetError::GraphicsUnavailable);
    CHECK(manager.load_font("broken.ttf", 16).error() == AssetError::GraphicsUnavailable);
    CHECK(manager.load_sound("silence.wav").error() == AssetError::AudioUnavailable);
    CHECK(manager.load_music("silence.wav").error() == AssetError::AudioUnavailable);
    CHECK(manager.loaded_count() == 0);
}

// Needs a display; skipped on a machine without one.
TEST_CASE("a texture is loaded once and then taken from the cache")
{
    Application app;
    if (!add_platform(app, false))
    {
        SKIP("no window can be opened on this machine");
    }
    app.add_plugin(AssetsPlugin{});
    add_test_files(app.world().resource<AssetCatalog>());
    AssetManager& manager = app.world().resource<AssetManager>();

    const auto first = manager.load_texture("pixel.bmp");
    const auto second = manager.load_texture("pixel.bmp");

    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    CHECK(*first == *second);
    CHECK(app.world().resource<Graphics>().texture_size(*first) == Vec2{2.0f, 1.0f});
    CHECK(manager.loaded_count() == 1);
}

// Needs a display; skipped on a machine without one.
TEST_CASE("corrupt files and missing audio are reported")
{
    Application app;
    if (!add_platform(app, false))
    {
        SKIP("no window can be opened on this machine");
    }
    app.add_plugin(AssetsPlugin{});
    add_test_files(app.world().resource<AssetCatalog>());
    AssetManager& manager = app.world().resource<AssetManager>();

    CHECK(manager.load_texture("broken.png").error() == AssetError::InvalidData);
    CHECK(manager.load_font("broken.ttf", 16).error() == AssetError::InvalidData);
    CHECK(manager.load_sound("silence.wav").error() == AssetError::AudioUnavailable);
    CHECK(manager.loaded_count() == 0);
}

// Needs a display; skipped on a machine without one.
TEST_CASE("unloaded textures become stale and are loaded again on request")
{
    Application app;
    if (!add_platform(app, false))
    {
        SKIP("no window can be opened on this machine");
    }
    app.add_plugin(AssetsPlugin{});
    add_test_files(app.world().resource<AssetCatalog>());
    AssetManager& manager = app.world().resource<AssetManager>();
    const Graphics& graphics = app.world().resource<Graphics>();
    const Handle<Texture> old = manager.load_texture("pixel.bmp").value();

    SECTION("one name")
    {
        manager.unload("pixel.bmp");
    }
    SECTION("everything")
    {
        manager.clear();
    }

    CHECK(manager.loaded_count() == 0);
    CHECK_FALSE(graphics.texture_size(old).has_value());
    const auto reloaded = manager.load_texture("pixel.bmp");
    REQUIRE(reloaded.has_value());
    CHECK(*reloaded != old);
    CHECK(graphics.texture_size(*reloaded) == Vec2{2.0f, 1.0f});
    CHECK_NOTHROW(manager.unload("never-loaded.png"));
}

// Needs a display and an audio device; skipped on a machine without them.
TEST_CASE("sounds and music are loaded once and can be unloaded")
{
    Application app;
    if (!add_platform(app, true))
    {
        SKIP("no window can be opened on this machine");
    }
    if (app.world().try_resource<ember::platform::Audio>() == nullptr)
    {
        SKIP("no audio device can be opened on this machine");
    }
    app.add_plugin(AssetsPlugin{});
    add_test_files(app.world().resource<AssetCatalog>());
    AssetManager& manager = app.world().resource<AssetManager>();

    const auto sound = manager.load_sound("silence.wav");
    const auto music = manager.load_music("silence.wav");
    REQUIRE(sound.has_value());
    REQUIRE(music.has_value());
    CHECK(manager.load_sound("silence.wav").value() == *sound);
    CHECK(manager.load_music("silence.wav").value() == *music);
    CHECK(manager.loaded_count() == 2);

    // The sound and the music share the name, so both are unloaded.
    manager.unload("silence.wav");
    CHECK(manager.loaded_count() == 0);
}
