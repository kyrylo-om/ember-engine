export module ember.ecs:registry;

import :entity;
import :world;
import std;

export namespace ember::ecs
{

/// Description of a registered component type, for tools that handle components by name:
/// scene loading and saving, and the debug inspector.
struct ComponentInfo
{
    // TODO: add to_json and from_json once ember.common exports ember::json

    /// Name of the component type without namespaces, for example "Transform".
    std::string_view name{};
    /// Returns true if the entity is alive and has a component of this type.
    bool (*has)(const World& world, Entity entity) = nullptr;
};

/// Stores a `ComponentInfo` for every registered component type. Stored as a world resource.
class ComponentRegistry
{
public:
    /// Registers a component type. Registering a type again does nothing.
    /// @tparam T Type of the component.
    template <typename T>
    void register_component();

    /// Returns the component type with the given name.
    /// @param name Name of the type without namespaces, for example "Transform".
    /// @return The description, or nullptr if no type with this name is registered.
    const ComponentInfo* find(std::string_view name) const;

    /// Returns all registered component types in registration order.
    std::span<const ComponentInfo> components() const;
};

} // namespace ember::ecs
