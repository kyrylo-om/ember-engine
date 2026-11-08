// What the entity inspector knows about entities. The world cannot list all its entities, so
// the inspector shows those with an ecs::Name, and only the names of their components: their
// values need reflection, which the component registry does not provide yet.
export module ember.debug:inspector;

import ember.ecs;
import std;

export namespace ember::debug
{

/// What the inspector shows about one entity.
struct EntitySummary
{
    /// The entity.
    ecs::Entity entity{};
    /// Value of its `ecs::Name`; empty if it has none.
    std::string name{};
    /// Names of its components registered in the `ecs::ComponentRegistry`, in registration
    /// order.
    std::vector<std::string_view> components{};
};

/// Returns all entities that have an `ecs::Name`, which are the entities the inspector lists.
/// @param world World with the entities.
/// @return The entities, sorted by name.
std::vector<ecs::Entity> named_entities(const ecs::World& world);

/// Returns all entities whose `ecs::Name` equals a name.
/// @param world World with the entities.
/// @param name Name to look for.
/// @return The entities with this name; empty if there are none.
std::vector<ecs::Entity> find_entities(const ecs::World& world, std::string_view name);

/// Describes an entity for the inspector.
/// @param world World with the entity and, optionally, an `ecs::ComponentRegistry` resource.
/// Without a registry the summary lists no components.
/// @param entity Entity to describe.
/// @return The summary, or `std::nullopt` if the entity is not alive.
[[nodiscard]] std::optional<EntitySummary> inspect(const ecs::World& world, ecs::Entity entity);

} // namespace ember::debug
