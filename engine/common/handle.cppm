export module ember.common:handle;

import std;

export namespace ember
{

/// Tag for handles to textures.
struct Texture
{
};

/// Tag for handles to sound effects, which are loaded into memory as a whole.
struct Sound
{
};

/// Tag for handles to music, which is streamed while it plays.
struct Music
{
};

/// Tag for handles to fonts.
struct Font
{
};

/// Typed ID of a resource owned by a service, such as a texture created by platform.
/// Many handles can refer to the same resource. A default-constructed handle refers to
/// nothing, because generation 0 is never given to a resource.
/// @tparam T Empty tag type naming the kind of resource: Texture, Sound, Music or Font.
/// @note A handle does not own its resource. After the resource is freed, its owner detects
/// the handle as stale by its generation.
template <typename T>
struct Handle
{
    /// Slot of the resource in the owner's table.
    std::uint32_t index = 0;
    /// Generation of the slot when the handle was created.
    std::uint32_t generation = 0;

    /// Compares two handles field by field.
    constexpr bool operator==(const Handle&) const = default;
};

} // namespace ember
