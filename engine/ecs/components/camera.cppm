export module ember.ecs:camera;

import ember.common;

export namespace ember::ecs
{

/// Part of the world that is drawn to the screen.
struct Camera
{
    /// Point of the world shown at the center of the screen, in world units.
    Vec2 position{};
    /// Scale of the view: at 1, one world unit is one screen pixel; at 2, everything is
    /// drawn twice as large.
    float zoom = 1.0f;
    /// Rotation of the view in radians, clockwise.
    float rotation = 0.0f;
};

} // namespace ember::ecs
