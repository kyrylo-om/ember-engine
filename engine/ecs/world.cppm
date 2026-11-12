export module ember.ecs:world;

import :commands;
import :entity;
import :storage;
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

private:
    friend class Commands;

    // Takes an index for a new entity without making the entity alive.
    Entity reserve_entity();

    // Makes a reserved entity alive.
    void activate(Entity entity);

    template <typename T>
    SparseSet<T>* find_set();

    template <typename T>
    const SparseSet<T>* find_set() const;

    // Generation of the entity that uses or will next use each index.
    std::vector<std::uint32_t> m_generations{};
    // False for the indices of destroyed entities and of entities that are only reserved.
    std::vector<bool> m_is_alive{};
    std::vector<std::uint32_t> m_free_indices{};
    std::unordered_map<std::type_index, std::unique_ptr<SparseSetBase>> m_sets{};
    // In insertion order, which the destructor needs.
    std::vector<std::unique_ptr<ResourceBase>> m_resources{};
    // Position of the resource of every type in m_resources.
    std::unordered_map<std::type_index, std::size_t> m_resource_slots{};
    Commands m_commands;
};

} // namespace ember::ecs

namespace ember::ecs
{

template <typename T>
T& World::add(Entity entity, T component)
{
    contract_assert(is_alive(entity));
    const auto [position, is_inserted] = m_sets.try_emplace(std::type_index{typeid(T)});
    if (is_inserted)
    {
        position->second = std::make_unique<SparseSet<T>>();
    }
    return static_cast<SparseSet<T>&>(*position->second).emplace(entity, std::move(component));
}

template <typename T>
T& World::get(Entity entity)
{
    T* component = try_get<T>(entity);
    contract_assert(component != nullptr);
    return *component;
}

template <typename T>
const T& World::get(Entity entity) const
{
    const T* component = try_get<T>(entity);
    contract_assert(component != nullptr);
    return *component;
}

template <typename T>
T* World::try_get(Entity entity)
{
    SparseSet<T>* set = find_set<T>();
    return set == nullptr ? nullptr : set->try_get(entity);
}

template <typename T>
const T* World::try_get(Entity entity) const
{
    const SparseSet<T>* set = find_set<T>();
    return set == nullptr ? nullptr : set->try_get(entity);
}

template <typename T>
bool World::has(Entity entity) const
{
    const SparseSet<T>* set = find_set<T>();
    return set != nullptr && set->contains(entity);
}

template <typename T>
void World::remove(Entity entity)
{
    if (SparseSet<T>* set = find_set<T>(); set != nullptr)
    {
        set->remove(entity);
    }
}

template <typename... Ts>
View<Ts...> World::view()
{
    return View<Ts...>{find_set<std::remove_const_t<Ts>>()...};
}

template <typename... Ts>
View<const Ts...> World::view() const
{
    return View<const Ts...>{find_set<std::remove_const_t<Ts>>()...};
}

template <typename T>
T& World::insert_resource(T resource)
{
    auto holder = std::make_unique<ResourceHolder<T>>(std::move(resource));
    T& stored = holder->value();
    const auto [position, is_inserted] =
        m_resource_slots.try_emplace(std::type_index{typeid(T)}, m_resources.size());
    if (is_inserted)
    {
        m_resources.push_back(std::move(holder));
    }
    else
    {
        // The new resource takes the place of the old one in the destruction order.
        m_resources[position->second] = std::move(holder);
    }
    return stored;
}

template <typename T>
T& World::resource()
{
    T* found = try_resource<T>();
    contract_assert(found != nullptr);
    return *found;
}

template <typename T>
const T& World::resource() const
{
    const T* found = try_resource<T>();
    contract_assert(found != nullptr);
    return *found;
}

template <typename T>
T* World::try_resource()
{
    const auto found = m_resource_slots.find(std::type_index{typeid(T)});
    if (found == m_resource_slots.end())
    {
        return nullptr;
    }
    return &static_cast<ResourceHolder<T>&>(*m_resources[found->second]).value();
}

template <typename T>
const T* World::try_resource() const
{
    const auto found = m_resource_slots.find(std::type_index{typeid(T)});
    if (found == m_resource_slots.end())
    {
        return nullptr;
    }
    return &static_cast<ResourceHolder<T>&>(*m_resources[found->second]).value();
}

template <typename T>
SparseSet<T>* World::find_set()
{
    const auto found = m_sets.find(std::type_index{typeid(T)});
    return found == m_sets.end() ? nullptr : static_cast<SparseSet<T>*>(found->second.get());
}

template <typename T>
const SparseSet<T>* World::find_set() const
{
    const auto found = m_sets.find(std::type_index{typeid(T)});
    return found == m_sets.end() ? nullptr : static_cast<const SparseSet<T>*>(found->second.get());
}

// Defined here and not next to Commands, because they use World, which the commands partition
// only declares.
template <typename T>
void Commands::add(Entity entity, T component)
{
    m_operations.emplace_back(
        [entity, component = std::move(component)](World& world) mutable
        {
            if (world.is_alive(entity))
            {
                world.add<T>(entity, std::move(component));
            }
        });
}

template <typename T>
void Commands::remove(Entity entity)
{
    m_operations.emplace_back([entity](World& world) { world.remove<T>(entity); });
}

} // namespace ember::ecs
