#include <catch_amalgamated.hpp>

import ember.common;
import ember.platform;
import std;

using Catch::Matchers::WithinAbs;
using ember::Color;
using ember::Font;
using ember::Handle;
using ember::Rect;
using ember::Texture;
using ember::Vec2;
using ember::platform::Graphics;
using ember::platform::Image;
using ember::platform::PlatformError;
using ember::platform::screen_to_world;
using ember::platform::TextureDraw;
using ember::platform::View;
using ember::platform::Window;
using ember::platform::world_to_screen;

namespace
{

constexpr float tolerance = 1e-3f;
constexpr float quarter_turn = std::numbers::pi_v<float> / 2.0f;

void check_point(Vec2 actual, Vec2 expected)
{
    CHECK_THAT(actual.x, WithinAbs(expected.x, tolerance));
    CHECK_THAT(actual.y, WithinAbs(expected.y, tolerance));
}

Image solid_image(int width, int height)
{
    return {.width = width,
            .height = height,
            .pixels = std::vector<Color>(static_cast<std::size_t>(width * height), Color::red)};
}

} // namespace

TEST_CASE("a view places its target at its offset")
{
    const View view{.target = {100.0f, 50.0f}, .offset = {640.0f, 360.0f}};

    check_point(world_to_screen(view, {100.0f, 50.0f}), {640.0f, 360.0f});
    check_point(world_to_screen(view, {110.0f, 45.0f}), {650.0f, 355.0f});
    check_point(screen_to_world(view, {640.0f, 360.0f}), {100.0f, 50.0f});
}

TEST_CASE("the zoom of a view scales distances from its target")
{
    const View view{.target = {100.0f, 50.0f}, .offset = {640.0f, 360.0f}, .zoom = 2.0f};

    check_point(world_to_screen(view, {110.0f, 50.0f}), {660.0f, 360.0f});
    check_point(screen_to_world(view, {660.0f, 360.0f}), {110.0f, 50.0f});
}

TEST_CASE("a view turned clockwise shows the world turned counterclockwise")
{
    const View view{.offset = {640.0f, 360.0f}, .rotation = quarter_turn};

    // With y pointing down, the point right of the target appears above it.
    check_point(world_to_screen(view, {10.0f, 0.0f}), {640.0f, 350.0f});
}

TEST_CASE("converting to the screen and back gives the same point")
{
    const View view{
        .target = {30.0f, -20.0f}, .offset = {400.0f, 300.0f}, .zoom = 1.5f, .rotation = 0.7f};
    const Vec2 point{12.0f, 34.0f};

    check_point(screen_to_world(view, world_to_screen(view, point)), point);
}

// The tests below need a graphics context. Each opens one window and checks everything in
// it, because opening a window is slow. On a machine without a display they are skipped.
TEST_CASE("a window is opened once, moved and closed")
{
    auto window = Window::open({.title = "Ember window test", .width = 320, .height = 240});
    if (!window.has_value())
    {
        SKIP("no window can be opened on this machine");
    }

    CHECK(window->size() == Vec2{320.0f, 240.0f});
    CHECK_FALSE(window->should_close());
    CHECK_FALSE(window->is_fullscreen());

    // Only one window can be open at a time.
    const auto second = Window::open({});
    REQUIRE_FALSE(second.has_value());
    CHECK(second.error() == PlatformError::WindowUnavailable);

    {
        const Window moved = std::move(*window);
        CHECK(moved.size() == Vec2{320.0f, 240.0f});
        CHECK_FALSE(Window::open({}).has_value());
    }
    // The new owner closed the window, so another one can be opened.
    CHECK(Window::open({.width = 100, .height = 100}).has_value());
}

TEST_CASE("graphics resources are created, found by handles and drawn")
{
    const auto window = Window::open({.title = "Ember graphics test", .width = 320, .height = 240});
    if (!window.has_value())
    {
        SKIP("no window can be opened on this machine");
    }
    Graphics graphics;

    // A texture is created from an image and found by its handle.
    const Handle<Texture> first = graphics.create_texture(solid_image(4, 2)).value();
    CHECK(graphics.texture_size(first) == Vec2{4.0f, 2.0f});
    CHECK_FALSE(graphics.texture_size(Handle<Texture>{}).has_value());

    // An image whose pixel count does not match its size is rejected.
    Image broken_image = solid_image(4, 2);
    broken_image.pixels.pop_back();
    const auto broken_texture = graphics.create_texture(broken_image);
    REQUIRE_FALSE(broken_texture.has_value());
    CHECK(broken_texture.error() == PlatformError::InvalidData);
    CHECK_FALSE(graphics.create_texture(Image{}).has_value());

    // A handle is stale after its texture is unloaded, even when the slot is reused.
    graphics.unload_texture(first);
    CHECK_FALSE(graphics.texture_size(first).has_value());
    const Handle<Texture> second = graphics.create_texture(solid_image(8, 8)).value();
    CHECK(second.index == first.index);
    CHECK(second != first);
    CHECK_FALSE(graphics.texture_size(first).has_value());
    CHECK(graphics.texture_size(second) == Vec2{8.0f, 8.0f});
    CHECK_NOTHROW(graphics.unload_texture(first));
    CHECK(graphics.texture_size(second).has_value());

    // Data that is not a font or an image is rejected.
    const std::vector<std::byte> garbage(64, std::byte{7});
    // Starts like a TrueType font with one table, which lies outside the data.
    std::vector<std::byte> truncated_font(64, std::byte{0xFF});
    std::ranges::copy(std::to_array<std::byte>({std::byte{0}, std::byte{1}, std::byte{0},
                                                std::byte{0}, std::byte{0}, std::byte{1}}),
                      truncated_font.begin());
    const auto font = graphics.load_font(garbage, ".ttf", 16);
    REQUIRE_FALSE(font.has_value());
    CHECK(font.error() == PlatformError::InvalidData);
    CHECK_FALSE(graphics.load_font(truncated_font, ".ttf", 16).has_value());
    CHECK_FALSE(graphics.load_font({}, ".ttf", 16).has_value());
    CHECK_FALSE(graphics.load_texture(garbage, ".png").has_value());

    // Text is measured with the built-in font.
    const Vec2 short_text = graphics.measure_text("ab", 20.0f);
    const Vec2 long_text = graphics.measure_text("abab", 20.0f);
    CHECK(short_text.x > 0.0f);
    CHECK(long_text.x > short_text.x);
    CHECK_THAT(short_text.y, WithinAbs(20.0f, tolerance));
    CHECK(graphics.measure_text("", 20.0f).x == 0.0f);

    // A frame with every kind of drawing is drawn without errors.
    const Rect rect{.position = {10.0f, 10.0f}, .size = {50.0f, 30.0f}};
    graphics.begin_frame(Color::black);
    graphics.begin_view({.offset = {160.0f, 120.0f}});
    graphics.draw_texture(second, {.position = {0.0f, 0.0f}, .scale = {4.0f, 4.0f}});
    graphics.draw_texture(
        second, {.source = {.size = {2.0f, 2.0f}}, .rotation = quarter_turn, .is_flipped_x = true});
    graphics.draw_texture(Handle<Texture>{}, {});
    // A second view replaces the first one.
    graphics.begin_view({.zoom = 2.0f});
    graphics.draw_circle({20.0f, 20.0f}, 10.0f, Color::green);
    graphics.end_view();
    graphics.draw_rect(rect, Color::blue);
    graphics.draw_rect_outline(rect, 2.0f, Color::white);
    graphics.draw_circle_outline({100.0f, 100.0f}, 15.0f, Color::red);
    graphics.draw_line({0.0f, 0.0f}, {320.0f, 240.0f}, 3.0f, Color::white);
    graphics.draw_text("Ember", {10.0f, 200.0f}, 20.0f, Color::white);
    graphics.draw_text("stale font", {10.0f, 220.0f}, 10.0f, Color::white, Handle<Font>{9, 9});
    graphics.end_view();
    graphics.end_frame();

    // Textures stay valid when the graphics service is moved.
    const Graphics moved = std::move(graphics);
    CHECK(moved.texture_size(second) == Vec2{8.0f, 8.0f});
}
