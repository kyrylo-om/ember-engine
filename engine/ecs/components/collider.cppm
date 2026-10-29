export module ember.ecs:collider;

import ember.common;
import std;

export namespace ember::ecs
{

/// Shape of a collider.
enum class ColliderShape
{
    /// Axis-aligned box with the size `Collider::size`.
    Box,
    /// Circle with the radius `Collider::radius`.
    Circle
};

/// Collision shape of an entity, centered on `Transform::position` plus `offset`.
/// The shape is not rotated or scaled by the `Transform`.
struct Collider
{
    /// Kind of shape; only the size field of this kind is used.
    ColliderShape shape = ColliderShape::Box;
    /// Width and height of the box in world units.
    Vec2 size{};
    /// Radius of the circle in world units.
    float radius = 0.0f;
    /// Offset of the center of the shape from `Transform::position`, in world units.
    Vec2 offset{};
    /// Whether overlaps are only reported instead of pushing the bodies apart.
    bool is_trigger = false;
    /// Bit mask of the collision layers the collider belongs to.
    std::uint32_t layer = 1;
    /// Bit mask of the layers the collider collides with. Two colliders collide only if the
    /// layer of each one is in the mask of the other.
    std::uint32_t mask = std::numeric_limits<std::uint32_t>::max();
};

} // namespace ember::ecs
