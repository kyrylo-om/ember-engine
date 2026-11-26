#include <catch_amalgamated.hpp>

import ember.common;
import ember.core;
import ember.ecs;
import ember.physics;
import std;

using Catch::Approx;
using ember::Vec2;
using ember::core::Application;
using ember::core::ApplicationConfig;
using ember::core::Seconds;
using ember::core::Stage;
using ember::ecs::BodyType;
using ember::ecs::Collider;
using ember::ecs::ColliderShape;
using ember::ecs::Entity;
using ember::ecs::RigidBody;
using ember::ecs::Transform;
using ember::ecs::Velocity;
using ember::physics::PhysicsPlugin;
using ember::physics::PhysicsSettings;

namespace
{

// Exact in binary, so every update of the tests runs exactly one fixed step.
constexpr Seconds step{1.0f / 64.0f};

// Top of the ground added by add_ground; a box of size 40 rests with its center 20 above it.
constexpr float ground_top = 480.0f;
constexpr float resting_y = ground_top - 20.0f;
constexpr Vec2 box_size{40.0f, 40.0f};

// Applications that run one fixed step per update and simulate physics.
struct PhysicsApp
{
    Application app{ApplicationConfig{.fixed_step = step}};

    PhysicsApp()
    {
        app.add_plugin(PhysicsPlugin{});
    }

    void simulate(Seconds duration)
    {
        const auto step_count = static_cast<int>(duration / step);
        for (int index = 0; index < step_count; ++index)
        {
            app.update(step);
        }
    }

    Entity add_box(Vec2 position, RigidBody rigid_body = {})
    {
        const Entity entity = app.world().create();
        app.world().add<Transform>(entity, {.position = position});
        app.world().add<RigidBody>(entity, rigid_body);
        app.world().add<Collider>(entity, {.shape = ColliderShape::Box, .size = box_size});
        app.world().add<Velocity>(entity);
        return entity;
    }

    Entity add_ground(Collider collider = {.shape = ColliderShape::Box, .size = {2000.0f, 40.0f}})
    {
        const Entity entity = app.world().create();
        app.world().add<Transform>(entity, {.position = {0.0f, ground_top + 20.0f}});
        app.world().add<RigidBody>(entity, {.type = BodyType::Static});
        app.world().add<Collider>(entity, collider);
        return entity;
    }

    Transform& transform(Entity entity)
    {
        return app.world().get<Transform>(entity);
    }

    Velocity& velocity(Entity entity)
    {
        return app.world().get<Velocity>(entity);
    }
};

} // namespace

TEST_CASE("the plugin inserts its settings and adds one fixed update system")
{
    Application app;
    app.add_plugin(PhysicsPlugin{.settings = {.gravity = {0.0f, 500.0f}}});

    REQUIRE(app.world().try_resource<PhysicsSettings>() != nullptr);
    CHECK(app.world().resource<PhysicsSettings>().gravity == Vec2{0.0f, 500.0f});
    CHECK(app.scheduler().system_count(Stage::FixedUpdate) == 1);
}

TEST_CASE("a dynamic body falls with gravity")
{
    PhysicsApp physics;
    const Entity box = physics.add_box({0.0f, 0.0f});

    physics.simulate(Seconds{0.5f});

    // Free fall: 980 / 2 * 0.5^2.
    CHECK(physics.transform(box).position.y == Approx(122.5f).margin(5.0f));
    CHECK(physics.transform(box).position.x == Approx(0.0f));
    CHECK(physics.velocity(box).value.y == Approx(490.0f).margin(5.0f));
}

TEST_CASE("a body without a velocity component falls too")
{
    PhysicsApp physics;
    const Entity box = physics.add_box({0.0f, 0.0f});
    physics.app.world().remove<Velocity>(box);

    physics.simulate(Seconds{0.5f});

    CHECK(physics.transform(box).position.y > 100.0f);
}

TEST_CASE("a static body stays where it is")
{
    PhysicsApp physics;
    const Entity wall = physics.add_box({10.0f, 20.0f}, {.type = BodyType::Static});

    physics.simulate(Seconds{1.0f});

    CHECK(physics.transform(wall).position == Vec2{10.0f, 20.0f});
}

TEST_CASE("a box falls onto the ground and rests on it")
{
    PhysicsApp physics;
    physics.add_ground();
    const Entity box = physics.add_box({0.0f, 300.0f});

    physics.simulate(Seconds{3.0f});

    CHECK(physics.transform(box).position.y == Approx(resting_y).margin(1.0f));
    CHECK(physics.velocity(box).value.y == Approx(0.0f).margin(1.0f));
}

TEST_CASE("a circle rests on the ground")
{
    PhysicsApp physics;
    physics.add_ground();
    const Entity ball = physics.add_box({0.0f, 300.0f});
    physics.app.world().add<Collider>(ball, {.shape = ColliderShape::Circle, .radius = 20.0f});

    physics.simulate(Seconds{3.0f});

    CHECK(physics.transform(ball).position.y == Approx(resting_y).margin(1.0f));
}

TEST_CASE("a body with gravity scale 0 floats")
{
    PhysicsApp physics;
    const Entity balloon = physics.add_box({0.0f, 100.0f}, {.gravity_scale = 0.0f});

    physics.simulate(Seconds{1.0f});

    CHECK(physics.transform(balloon).position.y == Approx(100.0f));
}

TEST_CASE("a kinematic body moves with its velocity and ignores gravity")
{
    PhysicsApp physics;
    const Entity platform = physics.add_box({0.0f, 0.0f}, {.type = BodyType::Kinematic});
    physics.velocity(platform).value = {100.0f, 0.0f};

    physics.simulate(Seconds{1.0f});

    CHECK(physics.transform(platform).position.x == Approx(100.0f).margin(0.5f));
    CHECK(physics.transform(platform).position.y == Approx(0.0f));
}

TEST_CASE("a velocity set by the game is passed to the simulation")
{
    PhysicsApp physics;
    physics.add_ground();
    const Entity box = physics.add_box({0.0f, resting_y});
    physics.simulate(Seconds{1.0f});
    REQUIRE(physics.transform(box).position.y == Approx(resting_y).margin(1.0f));

    physics.velocity(box).value.y = -300.0f;
    physics.simulate(step * 4);

    CHECK(physics.transform(box).position.y < resting_y - 10.0f);
    CHECK(physics.velocity(box).value.y < 0.0f);
}

TEST_CASE("a body moved by the game continues from its new position")
{
    PhysicsApp physics;
    const Entity box = physics.add_box({0.0f, 0.0f}, {.gravity_scale = 0.0f});
    physics.simulate(Seconds{1.0f});

    physics.transform(box).position = {300.0f, 200.0f};
    physics.simulate(step);

    CHECK(physics.transform(box).position.x == Approx(300.0f));
    CHECK(physics.transform(box).position.y == Approx(200.0f));
}

TEST_CASE("a body leaves the simulation with its entity or one of its components")
{
    PhysicsApp physics;
    const Entity platform = physics.add_box({0.0f, 200.0f}, {.type = BodyType::Static});
    const Entity box = physics.add_box({0.0f, 100.0f});
    physics.simulate(Seconds{1.0f});
    REQUIRE(physics.transform(box).position.y == Approx(160.0f).margin(1.0f));

    SECTION("when the entity is destroyed")
    {
        physics.app.world().destroy(platform);
    }
    SECTION("when its collider is removed")
    {
        physics.app.world().remove<Collider>(platform);
    }
    SECTION("when its rigid body is removed")
    {
        physics.app.world().remove<RigidBody>(platform);
    }
    physics.simulate(Seconds{1.0f});

    CHECK(physics.transform(box).position.y > 300.0f);
}

TEST_CASE("a tilted box topples onto its side unless its rotation is fixed")
{
    constexpr float tilt = 0.4f;
    PhysicsApp physics;
    physics.add_ground();

    SECTION("free to rotate")
    {
        const Entity box = physics.add_box({0.0f, 300.0f});
        physics.transform(box).rotation = tilt;
        physics.simulate(Seconds{3.0f});

        CHECK(std::abs(physics.transform(box).rotation - tilt) > 0.2f);
    }
    SECTION("rotation fixed")
    {
        const Entity box = physics.add_box({0.0f, 300.0f}, {.is_rotation_fixed = true});
        physics.transform(box).rotation = tilt;
        physics.simulate(Seconds{3.0f});

        CHECK(physics.transform(box).rotation == Approx(tilt));
    }
}

TEST_CASE("changing gravity while the game runs moves resting bodies")
{
    PhysicsApp physics;
    physics.add_ground();
    const Entity box = physics.add_box({0.0f, resting_y});
    // Long enough for the box to fall asleep on the ground.
    physics.simulate(Seconds{3.0f});

    physics.app.world().resource<PhysicsSettings>().gravity = {0.0f, -980.0f};
    physics.simulate(Seconds{0.5f});

    CHECK(physics.transform(box).position.y < resting_y - 50.0f);
}

TEST_CASE("changing the type of a body takes effect in the next step")
{
    PhysicsApp physics;
    const Entity box = physics.add_box({0.0f, 0.0f}, {.type = BodyType::Static});
    physics.simulate(Seconds{0.5f});
    REQUIRE(physics.transform(box).position.y == Approx(0.0f));

    physics.app.world().get<RigidBody>(box).type = BodyType::Dynamic;
    physics.simulate(Seconds{0.5f});

    CHECK(physics.transform(box).position.y > 100.0f);
}

TEST_CASE("colliders whose layers do not match pass through each other")
{
    PhysicsApp physics;
    physics.add_ground({.shape = ColliderShape::Box, .size = {2000.0f, 40.0f}, .mask = 1});
    const Entity box = physics.add_box({0.0f, 300.0f});
    physics.app.world().get<Collider>(box).layer = 2;

    physics.simulate(Seconds{2.0f});

    CHECK(physics.transform(box).position.y > ground_top + 100.0f);
}

TEST_CASE("a body falls through a trigger")
{
    PhysicsApp physics;
    physics.add_ground({.shape = ColliderShape::Box, .size = {2000.0f, 40.0f}, .is_trigger = true});
    const Entity box = physics.add_box({0.0f, 300.0f});

    physics.simulate(Seconds{2.0f});

    CHECK(physics.transform(box).position.y > ground_top + 100.0f);
}
