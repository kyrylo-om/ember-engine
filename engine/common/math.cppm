export module ember.common:math;

import std;

export namespace ember
{

/// Two-dimensional vector of floats, used for positions, directions, velocities and sizes.
struct Vec2
{
    /// Horizontal component; positive values point right.
    float x = 0.0f;
    /// Vertical component; positive values point down.
    float y = 0.0f;

    /// Adds another vector to this one.
    constexpr Vec2& operator+=(Vec2 other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    /// Subtracts another vector from this one.
    constexpr Vec2& operator-=(Vec2 other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    /// Multiplies both components by a scalar.
    constexpr Vec2& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    /// Divides both components by a scalar.
    constexpr Vec2& operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    /// Returns the squared length; cheaper than length() when only comparing lengths.
    constexpr float length_squared() const
    {
        return x * x + y * y;
    }

    /// Returns the length.
    float length() const
    {
        return std::sqrt(length_squared());
    }

    /// Returns a vector with the same direction and length 1.
    /// @return Unit vector, or the zero vector if this vector has length 0.
    Vec2 normalized() const;

    /// Compares two vectors component by component.
    constexpr bool operator==(const Vec2&) const = default;
};

/// Returns the component-wise sum of two vectors.
constexpr Vec2 operator+(Vec2 left, Vec2 right)
{
    return left += right;
}

/// Returns the component-wise difference of two vectors.
constexpr Vec2 operator-(Vec2 left, Vec2 right)
{
    return left -= right;
}

/// Returns the vector pointing in the opposite direction.
constexpr Vec2 operator-(Vec2 vector)
{
    return {-vector.x, -vector.y};
}

/// Returns the vector with both components multiplied by a scalar.
constexpr Vec2 operator*(Vec2 vector, float scalar)
{
    return vector *= scalar;
}

/// Returns the vector with both components multiplied by a scalar.
constexpr Vec2 operator*(float scalar, Vec2 vector)
{
    return vector *= scalar;
}

/// Returns the vector with both components divided by a scalar.
constexpr Vec2 operator/(Vec2 vector, float scalar)
{
    return vector /= scalar;
}

/// Returns the dot product of two vectors.
/// @param left First vector.
/// @param right Second vector.
/// @return Sum of the products of the components.
constexpr float dot(Vec2 left, Vec2 right)
{
    return left.x * right.x + left.y * right.y;
}

/// Returns the distance between two points.
/// @param from First point.
/// @param to Second point.
/// @return Length of the vector from one point to the other.
float distance(Vec2 from, Vec2 to)
{
    return (to - from).length();
}

/// Axis-aligned rectangle, for example a source region of a texture in pixels.
struct Rect
{
    /// Top-left corner.
    Vec2 position{};
    /// Width and height; both are non-negative.
    Vec2 size{};

    /// Compares two rectangles field by field.
    constexpr bool operator==(const Rect&) const = default;
};

/// Color with 8-bit red, green, blue and alpha channels.
struct Color
{
    /// Red channel, 0 to 255.
    std::uint8_t r = 0;
    /// Green channel, 0 to 255.
    std::uint8_t g = 0;
    /// Blue channel, 0 to 255.
    std::uint8_t b = 0;
    /// Alpha channel, from 0 (fully transparent) to 255 (opaque).
    std::uint8_t a = 255;

    /// Opaque white.
    static const Color white;
    /// Opaque black.
    static const Color black;
    /// Opaque red.
    static const Color red;
    /// Opaque green.
    static const Color green;
    /// Opaque blue.
    static const Color blue;
    /// Fully transparent black.
    static const Color transparent;

    /// Compares two colors channel by channel.
    constexpr bool operator==(const Color&) const = default;
};

} // namespace ember

namespace ember
{

// Defined outside the class because a class cannot hold constexpr instances of itself until
// it is complete.
inline constexpr Color Color::white{255, 255, 255, 255};
inline constexpr Color Color::black{0, 0, 0, 255};
inline constexpr Color Color::red{255, 0, 0, 255};
inline constexpr Color Color::green{0, 255, 0, 255};
inline constexpr Color Color::blue{0, 0, 255, 255};
inline constexpr Color Color::transparent{0, 0, 0, 0};

} // namespace ember
