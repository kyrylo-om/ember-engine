module ember.physics;

import :simulation;
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

    simulation.remove_bodies_if([&world](ecs::Entity entity) { return !is_body(world, entity); });
    for (auto [entity, transform, rigid_body, collider] :
         world.view<const ecs::Transform, const ecs::RigidBody, const ecs::Collider>())
    {
        simulation.sync_body(entity, transform, rigid_body, collider,
                             world.try_get<ecs::Velocity>(entity));
    }

    simulation.step(world.resource<core::Time>().fixed_delta, settings.gravity,
                    settings.substep_count);

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

} // namespace ember::physics
