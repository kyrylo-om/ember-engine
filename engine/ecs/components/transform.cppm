export module ember.ecs:transform;

import ember.common;

export namespace ember::ecs
{

/// Position, rotation and scale of an entity in the world.
struct Transform
{
    /// Center of the entity in world units; y points down.
    Vec2 position{};
    /// Rotation around the center in radians, clockwise.
    float rotation = 0.0f;
    /// Scale factor on each axis; 1 keeps the original size.
    Vec2 scale{1.0f, 1.0f};
};

} // namespace ember::ecs
