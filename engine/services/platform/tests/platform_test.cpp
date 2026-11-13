#include <catch_amalgamated.hpp>

import ember.common;
import ember.core;
import ember.ecs;
import ember.platform;
import std;

using ember::Color;
using ember::Vec2;
using ember::core::Application;
using ember::core::Seconds;
using ember::core::Stage;
using ember::ecs::World;
using ember::platform::Audio;
using ember::platform::Graphics;
using ember::platform::Input;
using ember::platform::Key;
using ember::platform::PlatformPlugin;
using ember::platform::Window;

namespace
{

constexpr Seconds frame{1.0f / 60.0f};

} // namespace

// Needs a display, like the graphics tests; skipped on a machine without one.
TEST_CASE("the platform plugin opens the window and runs frames")
{
    Application app;
    app.add_plugin(PlatformPlugin{
        .settings = {.window = {.title = "Ember platform test", .width = 320, .height = 240},
                     .clear_color = Color::blue,
                     .is_audio_enabled = false}});
    if (app.world().try_resource<Window>() == nullptr)
    {
        // Without a window the plugin asks the application to exit.
        CHECK_FALSE(app.is_running());
        SKIP("no window can be opened on this machine");
    }

    // The resources are inserted and the systems are in their stages.
    CHECK(app.is_running());
    CHECK(app.world().resource<Window>().size() == Vec2{320.0f, 240.0f});
    CHECK(app.world().try_resource<Graphics>() != nullptr);
    CHECK(app.world().try_resource<Input>() != nullptr);
    CHECK(app.world().try_resource<Audio>() == nullptr);
    CHECK(app.scheduler().system_count(Stage::PreUpdate) == 3);
    CHECK(app.scheduler().system_count(Stage::PreRender) == 1);
    CHECK(app.scheduler().system_count(Stage::PostRender) == 1);

    // A system in Stage::Render draws between the start and the end of the frame, and input
    // is readable in Stage::Update.
    int drawn_frames = 0;
    bool is_any_key_down = true;
    app.add_system(Stage::Update, "read", [&is_any_key_down](World& world)
                   { is_any_key_down = world.resource<Input>().is_key_down(Key::Space); });
    app.add_system(Stage::Render, "draw",
                   [&drawn_frames](World& world)
                   {
                       world.resource<Graphics>().draw_circle({160.0f, 120.0f}, 40.0f,
                                                              Color::white);
                       ++drawn_frames;
                   });

    app.update(frame);
    app.update(frame);

    CHECK(drawn_frames == 2);
    CHECK_FALSE(is_any_key_down);
    // Nobody asked to close the window, so the application keeps running.
    CHECK(app.is_running());
}
