export module ember.ecs:world;

import :commands;
import :entity;
import :view;
import std;

export namespace ember::ecs
{

/// Stores all entities, their components and the resources.
///
/// A component can be any plain struct, including types defined by the game. Adding,
/// getting and removing a component take constant time. A resource is a single object of
/// its type, such as a service, settings or `Time`; systems find services through them.
///
/// Functions that need something to exist (a live entity, a component, a resource) check it
/// only in debug builds. Use the `try_` variants where it may be missing.
/// @note Resources are destroyed in reverse order of insertion when the world is destroyed.
class World
{
public:
    /// Creates a world without entities and resources.
    World();
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    /// Creates an entity without components.
    /// @return The new entity.
    Entity create();

    /// Destroys an entity and removes all its components. Its index can be reused later.
    /// @param entity Entity to destroy. Nothing happens if it is not alive.
    void destroy(Entity entity);

    /// Returns true if the entity was created by this world and is not destroyed yet. False
    /// for the null entity and for stale entities.
    bool is_alive(Entity entity) const;

    /// Adds a component to an entity, replacing the component it already has.
    /// @tparam T Type of the component.
    /// @param entity Live entity that gets the component.
    /// @param component The component to add.
    /// @return The stored component.
    /// @note Invalidates references to other components of type T.
    template <typename T>
    T& add(Entity entity, T component = {});

    /// Returns a component of an entity.
    /// @tparam T Type of the component.
    /// @param entity Live entity that has the component.
    /// @return The component.
    template <typename T>
    T& get(Entity entity);

    /// Returns a component of an entity.
    /// @tparam T Type of the component.
    /// @param entity Live entity that has the component.
    /// @return The component.
    template <typename T>
    const T& get(Entity entity) const;

    /// Returns a component of an entity if it has one.
    /// @tparam T Type of the component.
    /// @param entity Entity whose component is returned.
    /// @return The component, or nullptr if the entity is not alive or has no such component.
    template <typename T>
    T* try_get(Entity entity);

    /// Returns a component of an entity if it has one.
    /// @tparam T Type of the component.
    /// @param entity Entity whose component is returned.
    /// @return The component, or nullptr if the entity is not alive or has no such component.
    template <typename T>
    const T* try_get(Entity entity) const;

    /// Returns true if the entity is alive and has a component of type T.
    template <typename T>
    bool has(Entity entity) const;

    /// Removes a component from an entity.
    /// @tparam T Type of the component.
    /// @param entity Entity that loses the component. Nothing happens if it is not alive or
    /// has no such component.
    template <typename T>
    void remove(Entity entity);

    /// Returns a view over all entities that have all the given components.
    /// @tparam Ts Component types.
    template <typename... Ts>
    View<Ts...> view();

    /// Returns a view over all entities that have all the given components.
    /// @tparam Ts Component types.
    template <typename... Ts>
    View<const Ts...> view() const;

    /// Inserts a resource, replacing the resource of the same type if there is one.
    /// @tparam T Type of the resource.
    /// @param resource The resource to insert.
    /// @return The stored resource.
    template <typename T>
    T& insert_resource(T resource);

    /// Returns a resource that must exist.
    /// @tparam T Type of the resource.
    template <typename T>
    T& resource();

    /// Returns a resource that must exist.
    /// @tparam T Type of the resource.
    template <typename T>
    const T& resource() const;

    /// Returns a resource, or nullptr if there is none of type T.
    /// @tparam T Type of the resource.
    template <typename T>
    T* try_resource();

    /// Returns a resource, or nullptr if there is none of type T.
    /// @tparam T Type of the resource.
    template <typename T>
    const T* try_resource() const;

    /// Returns the command list whose changes are applied by `apply_commands()`.
    Commands& commands();

    /// Applies all recorded commands in recording order and clears the list.
    void apply_commands();
};

} // namespace ember::ecs
