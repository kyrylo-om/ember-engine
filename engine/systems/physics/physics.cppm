// 2D physics for the RigidBody and Collider components of ecs: settings, collision and trigger
// events, the plugin that steps the simulation, raycasts and impulses.
export module ember.physics;

export import :events;
export import :raycast_hit;

import ember.common;
import ember.core;
import ember.ecs;
import std;

export namespace ember::physics
{

/// Settings of the physics simulation, stored as a world resource.
///
/// `gravity` and `substep_count` can be changed while the game runs; the change takes effect
/// in the next step.
struct PhysicsSettings
{
    /// Acceleration of every dynamic body in world units per second squared, multiplied by
    /// `ecs::RigidBody::gravity_scale`; y points down.
    Vec2 gravity{0.0f, 980.0f};
    /// Number of world units in one meter. The simulation is most accurate for moving bodies
    /// from 0.1 to 10 meters in size, so choose a scale that makes a typical body about one
    /// meter.
    /// @note Must not change after the plugin is built, because the bodies already created
    /// keep their size in meters.
    float units_per_meter = 100.0f;
    /// Number of substeps in every fixed step. More substeps make stacks of bodies and fast
    /// bodies more accurate, but take more time.
    int substep_count = 4;
};

/// Simulates rigid bodies in `core::Stage::FixedUpdate` and publishes collision and trigger
/// events.
///
/// Every entity with an `ecs::Transform`, an `ecs::RigidBody` and an `ecs::Collider` is a body.
/// It joins the simulation in the first step after it has all three components and leaves it
/// when it loses one of them or is destroyed. An `ecs::Velocity` is optional: without one, a
/// body starts at rest and its velocity cannot be read or set.
///
/// In every step:
/// - changes that the game made to `ecs::Transform`, `ecs::Velocity`, `ecs::RigidBody` and
///   `ecs::Collider` since the previous step are passed to the simulation;
/// - impulses given to `apply_impulse()` since the previous step are applied;
/// - the simulation advances by `core::Time::fixed_delta`;
/// - the new position, rotation and velocity of every moving body are written back to its
///   `ecs::Transform` and `ecs::Velocity`;
/// - `CollisionStarted`, `CollisionEnded`, `TriggerEntered` and `TriggerExited` are published
///   on the `core::EventBus`, so they can be read in the next frame.
///
/// Needs no window, so it works in tests without the platform plugin.
struct PhysicsPlugin
{
    /// Settings inserted into the world as a resource.
    PhysicsSettings settings{};

    /// Inserts the settings and adds the system that steps the simulation.
    /// @param app Application to add the system to.
    void build(core::Application& app) const;
};

/// Finds the first collider on the line from one point to another.
/// @param world World with the physics plugin.
/// @param from Start of the ray in world units.
/// @param to End of the ray in world units.
/// @param mask Bit mask of the collision layers the ray can hit, like `ecs::Collider::mask`.
/// @return The hit closest to `from`, or nothing if the ray hits no collider or the world has
/// no physics plugin.
/// @note Sees the bodies as they were after the last fixed step, so it misses entities that
/// have not been in a step yet. Triggers and a collider that contains `from` are not hit.
[[nodiscard]] std::optional<RaycastHit>
raycast(const ecs::World& world, Vec2 from, Vec2 to,
        std::uint32_t mask = std::numeric_limits<std::uint32_t>::max());

/// Pushes a body like a hit does: changes its velocity by the impulse divided by its mass, so
/// a light body moves faster than a heavy one.
/// @param world World with the physics plugin. Nothing happens without one.
/// @param entity Entity whose body is pushed. Nothing happens if it is not a dynamic body in
/// the next fixed step.
/// @param impulse Impulse in kilograms times world units per second.
/// @note Applied in the next fixed step, after the changes made to the components, so it also
/// works for an entity created in the same frame.
void apply_impulse(ecs::World& world, ecs::Entity entity, Vec2 impulse);

} // namespace ember::physics
