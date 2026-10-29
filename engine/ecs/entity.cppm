export module ember.ecs:entity;

import std;

export namespace ember::ecs
{

/// Identifies an entity in a world. An entity has no data of its own; it is the key under
/// which its components are stored.
/// A default-constructed entity is the null entity, which never refers to a live entity,
/// because generation 0 is never given to an entity.
/// @note After an entity is destroyed, its index can be reused. The world detects the old
/// value as stale by its generation.
struct Entity
{
    /// Slot of the entity in the world.
    std::uint32_t index = 0;
    /// Generation of the slot when the entity was created.
    std::uint32_t generation = 0;

    /// Compares two entities field by field, so they can be sorted and used as map keys.
    constexpr auto operator<=>(const Entity&) const = default;
};

/// Entity that refers to nothing; equal to a default-constructed entity.
inline constexpr Entity null_entity{};

} // namespace ember::ecs
