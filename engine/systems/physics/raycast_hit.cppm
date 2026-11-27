export module ember.physics:raycast_hit;

import ember.common;
import ember.ecs;

export namespace ember::physics
{

/// Where a ray found by `raycast()` hit a collider.
struct RaycastHit
{
    /// Entity whose collider the ray hit.
    ecs::Entity entity{};
    /// Point on the surface of the collider where the ray hit it, in world units.
    Vec2 point{};
    /// Unit vector perpendicular to the surface at `point`, pointing out of the collider.
    Vec2 normal{};
    /// Distance from the start of the ray to `point`, in world units.
    float distance = 0.0f;
};

} // namespace ember::physics
