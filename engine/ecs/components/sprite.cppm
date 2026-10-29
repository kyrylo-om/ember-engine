export module ember.ecs:sprite;

import ember.common;

export namespace ember::ecs
{

/// Image drawn for an entity with a `Transform`.
///
/// The image is centered on `Transform::position`, rotated and scaled by the `Transform`.
/// Unscaled, one pixel of the texture is one world unit.
struct Sprite
{
    /// Texture to draw from. Nothing is drawn for a null handle.
    Handle<Texture> texture{};
    /// Part of the texture to draw, in pixels from its top-left corner. A size of 0 draws
    /// the whole texture.
    Rect source{};
    /// Color multiplied with the texture; white draws it unchanged.
    Color tint = Color::white;
    /// Draw order; sprites with a higher layer are drawn on top.
    int layer = 0;
    /// Whether the image is mirrored horizontally.
    bool is_flipped_x = false;
    /// Whether the image is mirrored vertically.
    bool is_flipped_y = false;
};

} // namespace ember::ecs
