export module ember.render:shape_queue;

import ember.common;
import std;

export namespace ember::render
{

/// Rectangle in world units, filled or as an outline.
struct RectShape
{
    /// The rectangle.
    Rect rect{};
    /// Width of the outline, which lies inside the rectangle; 0 fills the rectangle.
    float outline = 0.0f;
};

/// Circle in world units, filled or as an outline.
struct CircleShape
{
    /// Center of the circle.
    Vec2 center{};
    /// Radius of the circle.
    float radius = 0.0f;
    /// Whether only the outline is drawn, with a width of one pixel.
    bool is_outline = false;
};

/// Straight line in world units.
struct LineShape
{
    /// Start point.
    Vec2 from{};
    /// End point.
    Vec2 to{};
    /// Width of the line.
    float thickness = 1.0f;
};

/// Simple shape drawn by the render system together with the sprites.
struct Shape
{
    /// Kind, position and size of the shape.
    std::variant<RectShape, CircleShape, LineShape> geometry{};
    /// Color of the shape.
    Color color = Color::white;
    /// Draw order, compared with `ecs::Sprite::layer`; shapes with a higher layer are drawn
    /// on top.
    int layer = 0;
};

/// Shapes to draw in the next frame; a world resource inserted by the render plugin.
///
/// Systems add shapes before `Stage::Render`, for example a selection frame or a path. The
/// render system draws them sorted by layer together with the sprites and then clears the
/// queue, so a shape that stays visible is added every frame.
class ShapeQueue
{
public:
    /// Adds a shape to draw in this frame.
    /// @param shape The shape.
    void add(const Shape& shape);

    /// Returns the shapes added since the last `clear()`, in the order they were added.
    std::span<const Shape> shapes() const;

    /// Removes all shapes. The render system calls it after drawing them.
    void clear();

private:
    std::vector<Shape> m_shapes{};
};

} // namespace ember::render
