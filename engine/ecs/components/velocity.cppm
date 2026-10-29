export module ember.ecs:velocity;

import ember.common;

export namespace ember::ecs
{

/// Linear velocity of an entity.
struct Velocity
{
    /// Change of `Transform::position` in world units per second.
    Vec2 value{};
};

} // namespace ember::ecs
