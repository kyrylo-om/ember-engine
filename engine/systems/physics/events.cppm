export module ember.physics:events;

import ember.common;
import ember.ecs;

export namespace ember::physics
{

/// Published when two solid colliders start touching.
///
/// The order of the two entities is not specified, so a game that looks for a pair checks
/// both orders.
struct CollisionStarted
{
    /// One of the two entities.
    ecs::Entity first{};
    /// The other entity.
    ecs::Entity second{};
    /// Unit vector from `first` toward `second` at the contact. For example, when `second`
    /// is the ground below `first`, the normal points down and its y is positive.
    Vec2 normal{};
};

/// Published when two solid colliders that touched stop touching, also when one of them is
/// destroyed or stops being a body.
struct CollisionEnded
{
    /// One of the two entities.
    ecs::Entity first{};
    /// The other entity.
    ecs::Entity second{};
};

/// Published when a solid collider starts overlapping a trigger.
/// @note Triggers do not detect other triggers.
struct TriggerEntered
{
    /// Entity whose collider is the trigger.
    ecs::Entity trigger{};
    /// Entity that entered the trigger.
    ecs::Entity other{};
};

/// Published when a solid collider stops overlapping a trigger, also when one of them is
/// destroyed or stops being a body.
struct TriggerExited
{
    /// Entity whose collider is the trigger.
    ecs::Entity trigger{};
    /// Entity that left the trigger.
    ecs::Entity other{};
};

} // namespace ember::physics
