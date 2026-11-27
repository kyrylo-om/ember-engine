module ember.physics;

import :simulation;
import ember.common;
import ember.core;
import ember.ecs;
import std;

namespace ember::physics
{

namespace
{

bool is_body(const ecs::World& world, ecs::Entity entity)
{
    return world.has<ecs::Transform>(entity) && world.has<ecs::RigidBody>(entity) &&
           world.has<ecs::Collider>(entity);
}

void step_physics(ecs::World& world)
{
    const PhysicsSettings& settings = world.resource<PhysicsSettings>();
    Simulation& simulation = world.resource<Simulation>();
    core::EventBus& events = world.resource<core::EventBus>();

    simulation.remove_bodies_if([&world](ecs::Entity entity) { return !is_body(world, entity); },
                                events);
    for (auto [entity, transform, rigid_body, collider] :
         world.view<const ecs::Transform, const ecs::RigidBody, const ecs::Collider>())
    {
        simulation.sync_body(entity, transform, rigid_body, collider,
                             world.try_get<ecs::Velocity>(entity), events);
    }

    simulation.step(world.resource<core::Time>().fixed_delta, settings.gravity,
                    settings.substep_count, events);

    for (auto [entity, transform, rigid_body, collider] :
         world.view<ecs::Transform, const ecs::RigidBody, const ecs::Collider>())
    {
        simulation.write_body(entity, transform, world.try_get<ecs::Velocity>(entity));
    }
}

} // namespace

void PhysicsPlugin::build(core::Application& app) const
{
    app.world().insert_resource(settings);
    app.world().insert_resource(Simulation{settings.units_per_meter});
    app.add_system(core::Stage::FixedUpdate, "step_physics", step_physics);
}

std::optional<RaycastHit> raycast(const ecs::World& world, Vec2 from, Vec2 to, std::uint32_t mask)
{
    const Simulation* simulation = world.try_resource<Simulation>();
    if (simulation == nullptr)
    {
        return std::nullopt;
    }
    return simulation->raycast(from, to, mask);
}

void apply_impulse(ecs::World& world, ecs::Entity entity, Vec2 impulse)
{
    if (Simulation* simulation = world.try_resource<Simulation>(); simulation != nullptr)
    {
        simulation->queue_impulse(entity, impulse);
    }
}

} // namespace ember::physics
