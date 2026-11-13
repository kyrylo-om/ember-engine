#include <catch_amalgamated.hpp>

import ember.common;
import ember.core;
import ember.ecs;
import ember.particles;
import ember.platform;
import std;

using ember::Vec2;
using ember::core::Application;
using ember::core::Seconds;
using ember::core::Stage;
using ember::ecs::Emitter;
using ember::ecs::Entity;
using ember::ecs::Transform;
using ember::ecs::World;
using ember::particles::particle_count;
using ember::particles::ParticlePlugin;
using ember::particles::ParticleSettings;

namespace
{

// Frame time that is exact in binary, so particle counts in the tests have no rounding error.
constexpr Seconds frame{0.25f};

// An emitter that emits 8 particles per second, so 2 in every frame of the tests.
Emitter steady_emitter()
{
    Emitter emitter{};
    emitter.emission_rate = 8.0f;
    emitter.lifetime = Seconds{10.0f};
    return emitter;
}

Entity add_emitter(Application& app, Emitter emitter)
{
    const Entity entity = app.world().create();
    app.world().add<Transform>(entity);
    app.world().add<Emitter>(entity, emitter);
    return entity;
}

} // namespace

TEST_CASE("a world without the particle plugin has no particles")
{
    const World world;

    CHECK(particle_count(world) == 0);
}

TEST_CASE("the plugin inserts its settings and adds its two systems")
{
    Application app;
    app.add_plugin(ParticlePlugin{.settings = {.max_particles_per_emitter = 16}});

    REQUIRE(app.world().try_resource<ParticleSettings>() != nullptr);
    CHECK(app.world().resource<ParticleSettings>().max_particles_per_emitter == 16);
    CHECK(app.scheduler().system_count(Stage::Update) == 1);
    CHECK(app.scheduler().system_count(Stage::Render) == 1);
    CHECK(particle_count(app.world()) == 0);
}

TEST_CASE("an emitter emits particles every frame")
{
    Application app;
    app.add_plugin(ParticlePlugin{});
    add_emitter(app, steady_emitter());

    app.update(frame);
    CHECK(particle_count(app.world()) == 2);

    app.update(frame);
    CHECK(particle_count(app.world()) == 4);
}

TEST_CASE("particles of all emitters are counted")
{
    Application app;
    app.add_plugin(ParticlePlugin{});
    add_emitter(app, steady_emitter());
    add_emitter(app, steady_emitter());

    app.update(frame);

    CHECK(particle_count(app.world()) == 4);
}

TEST_CASE("an emitter without a transform emits nothing")
{
    Application app;
    app.add_plugin(ParticlePlugin{});
    const Entity entity = app.world().create();
    app.world().add<Emitter>(entity, steady_emitter());

    app.update(frame);

    CHECK(particle_count(app.world()) == 0);
}

TEST_CASE("a burst is emitted once and reset in the component")
{
    Application app;
    app.add_plugin(ParticlePlugin{});
    Emitter emitter = steady_emitter();
    emitter.is_emitting = false;
    emitter.burst_count = 5;
    const Entity entity = add_emitter(app, emitter);

    app.update(frame);
    CHECK(particle_count(app.world()) == 5);
    CHECK(app.world().get<Emitter>(entity).burst_count == 0);

    app.update(frame);
    CHECK(particle_count(app.world()) == 5);
}

TEST_CASE("particles are removed at the end of their lifetime")
{
    Application app;
    app.add_plugin(ParticlePlugin{});
    Emitter emitter = steady_emitter();
    emitter.is_emitting = false;
    emitter.burst_count = 3;
    emitter.lifetime = frame * 2;
    add_emitter(app, emitter);

    // Emitted with age 0 in the first frame, so they reach the lifetime two frames later.
    app.update(frame);
    app.update(frame);
    CHECK(particle_count(app.world()) == 3);

    app.update(frame);
    CHECK(particle_count(app.world()) == 0);
}

TEST_CASE("an emitter never has more particles than the configured maximum")
{
    Application app;
    app.add_plugin(ParticlePlugin{.settings = {.max_particles_per_emitter = 3}});
    add_emitter(app, steady_emitter());

    app.update(frame);
    app.update(frame);
    app.update(frame);

    CHECK(particle_count(app.world()) == 3);
}

TEST_CASE("the particles of an emitter disappear with it")
{
    Application app;
    app.add_plugin(ParticlePlugin{});
    const Entity kept = add_emitter(app, steady_emitter());
    const Entity changed = add_emitter(app, steady_emitter());
    app.update(frame);
    REQUIRE(particle_count(app.world()) == 4);

    SECTION("when the entity is destroyed")
    {
        app.world().destroy(changed);
        app.update(frame);
        CHECK(particle_count(app.world()) == 4);
    }

    SECTION("when the emitter component is removed")
    {
        app.world().remove<Emitter>(changed);
        app.update(frame);
        CHECK(particle_count(app.world()) == 4);
    }

    CHECK(app.world().is_alive(kept));
}

TEST_CASE("the simd and scalar simulation give the same number of particles")
{
    const bool use_simd = GENERATE(true, false);
    Application app;
    app.add_plugin(ParticlePlugin{.settings = {.use_simd = use_simd}});
    Emitter emitter = steady_emitter();
    emitter.emission_rate = 40.0f;
    emitter.lifetime = frame * 3;
    emitter.acceleration = Vec2{0.0f, 100.0f};
    add_emitter(app, emitter);

    for (int index = 0; index < 8; ++index)
    {
        app.update(frame);
    }

    // 10 particles per frame, alive during the frame they are emitted in and two more.
    CHECK(particle_count(app.world()) == 30);
}

// Needs a display; skipped on a machine without one.
TEST_CASE("particles are drawn in a window through a camera")
{
    Application app;
    app.add_plugin(ember::platform::PlatformPlugin{
        .settings = {.window = {.title = "Ember particles test", .width = 320, .height = 240},
                     .is_audio_enabled = false}});
    if (app.world().try_resource<ember::platform::Window>() == nullptr)
    {
        SKIP("no window can be opened on this machine");
    }
    app.add_plugin(ParticlePlugin{});

    Emitter emitter = steady_emitter();
    emitter.start_color = ember::Color::red;
    emitter.end_color = ember::Color::transparent;
    const Entity entity = add_emitter(app, emitter);
    const Entity camera = app.world().create();
    app.world().add<ember::ecs::Camera>(camera, {.position = {10.0f, 10.0f}, .zoom = 2.0f});

    app.update(frame);
    app.update(frame);
    CHECK(particle_count(app.world()) == 4);

    // A frame after the emitter is gone draws nothing and does not fail.
    app.world().destroy(entity);
    CHECK_NOTHROW(app.update(frame));
    CHECK(particle_count(app.world()) == 0);
}
