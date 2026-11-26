export module ember.ecs:velocity;

import ember.common;

export namespace ember::ecs
{

/// Linear and angular velocity of an entity.
struct Velocity
{
    /// Change of `Transform::position` in world units per second.
    Vec2 value{};
    /// Change of `Transform::rotation` in radians per second, clockwise.
    float angular = 0.0f;
};

} // namespace ember::ecs
