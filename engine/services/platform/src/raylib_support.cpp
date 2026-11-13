// Helpers shared by the sources that call Raylib: the Raylib window object and conversions
// between Ember and Raylib types.
module ember.platform:raylib_support;

import ember.common;
import raylib;
import std;

namespace ember::platform
{

// Returns the object through which Raylib's only window is used. Raylib keeps the window in
// global state, so the object holds no data; creating it does not open the window.
inline raylib::Window& raylib_window()
{
    static raylib::Window window;
    return window;
}

inline raylib::Vector2 to_raylib(Vec2 vector)
{
    return {vector.x, vector.y};
}

inline raylib::Color to_raylib(Color color)
{
    return {color.r, color.g, color.b, color.a};
}

inline raylib::Rectangle to_raylib(Rect rect)
{
    return {rect.position.x, rect.position.y, rect.size.x, rect.size.y};
}

inline Vec2 to_vec2(const raylib::Vector2& vector)
{
    return {vector.x, vector.y};
}

// Raylib takes angles in degrees; Ember uses radians everywhere.
inline float to_degrees(float radians)
{
    return radians * (180.0f / std::numbers::pi_v<float>);
}

} // namespace ember::platform
