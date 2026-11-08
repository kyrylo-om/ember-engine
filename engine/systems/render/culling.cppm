// Tests that decide which sprites are skipped because they are outside the view. They work
// with axis-aligned rectangles, so a rotated view or sprite is tested with the rectangle
// around it: nothing visible is skipped, but a few invisible sprites may be drawn.
export module ember.render:culling;

import ember.common;
import ember.ecs;
import ember.platform;
import std;

export namespace ember::render
{

/// Returns the part of the world visible through a view.
/// @param view View that maps the world to the window.
/// @param screen_size Size of the window in pixels.
/// @return Smallest axis-aligned rectangle in world units that contains everything visible.
Rect visible_area(const platform::View& view, Vec2 screen_size);

/// Returns the part of the world covered by a sprite.
/// @param transform Position, rotation and scale of the sprite.
/// @param image_size Size of the drawn part of the texture in pixels.
/// @return Smallest axis-aligned rectangle in world units that contains the rotated and
/// scaled image.
Rect sprite_bounds(const ecs::Transform& transform, Vec2 image_size);

/// Returns true if two rectangles overlap or touch.
/// @param first First rectangle.
/// @param second Second rectangle.
/// @return Whether the rectangles have at least one point in common.
bool overlaps(Rect first, Rect second);

} // namespace ember::render
