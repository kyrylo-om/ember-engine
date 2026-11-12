export module ember.ecs:view;

import :entity;
import :storage;
import std;

export namespace ember::ecs
{

class World;

/// Range over the entities that have all of the given components, created by
/// `World::view()`.
///
/// Every element is the entity followed by references to its components, so a view is used
/// with a structured binding:
/// `for (auto [entity, transform, velocity] : world.view<Transform, Velocity>())`.
/// Entities that miss any of the components are skipped.
/// @tparam Ts Component types. A `const` type gives `const` references.
/// @note Creating or destroying entities, or adding or removing components of the viewed
/// types, while iterating invalidates the view. Record such changes with `Commands`.
template <typename... Ts>
    requires(sizeof...(Ts) > 0)
class View
{
public:
    /// Element of the view: the entity and references to its components.
    using Element = std::tuple<Entity, Ts&...>;

private:
    // Set of a component type; const for a const type, so its components cannot be changed.
    template <typename T>
    using Set = std::conditional_t<std::is_const_v<T>, const SparseSet<std::remove_const_t<T>>,
                                   SparseSet<T>>;

public:

    /// Iterator over the elements of the view, compared with `std::default_sentinel` to find
    /// the end.
    class Iterator
    {
    public:
        /// Type of the elements.
        using value_type = Element;
        /// Type of the distance between two iterators.
        using difference_type = std::ptrdiff_t;

        /// Returns the current element.
        Element operator*() const;

        /// Moves to the next entity that has all the components.
        Iterator& operator++();

        /// Moves to the next entity that has all the components.
        void operator++(int);

        /// Returns true if the iterator is past the last element.
        bool operator==(std::default_sentinel_t) const;

    private:
        friend class View;

        Iterator(std::tuple<Set<Ts>*...> sets, std::span<const Entity> entities);

        // Moves forward until the current entity has all the components.
        void skip_incomplete();

        std::tuple<Set<Ts>*...> m_sets{};
        // Entities of the set that is iterated; the other sets are only checked.
        std::span<const Entity> m_entities{};
        std::size_t m_position = 0;
    };

    /// Returns an iterator to the first element.
    Iterator begin();

    /// Returns the sentinel that marks the end of the view.
    std::default_sentinel_t end() const;

private:
    friend class World;

    // A set is null if no component of its type was ever added.
    explicit View(Set<Ts>*... sets);

    std::tuple<Set<Ts>*...> m_sets{};
};

} // namespace ember::ecs

namespace ember::ecs
{

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
View<Ts...>::View(Set<Ts>*... sets) : m_sets{sets...}
{
}

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
auto View<Ts...>::begin() -> Iterator
{
    // Iterating the set with the fewest entities checks the fewest candidates.
    using Sets = std::array<const SparseSetBase*, sizeof...(Ts)>;
    const Sets sets = std::apply([](const Set<Ts>*... set) { return Sets{set...}; }, m_sets);
    const SparseSetBase* smallest = nullptr;
    for (const SparseSetBase* set : sets)
    {
        if (set == nullptr)
        {
            return Iterator{m_sets, {}};
        }
        if (smallest == nullptr || set->entities().size() < smallest->entities().size())
        {
            smallest = set;
        }
    }
    return Iterator{m_sets, smallest->entities()};
}

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
std::default_sentinel_t View<Ts...>::end() const
{
    return std::default_sentinel;
}

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
View<Ts...>::Iterator::Iterator(std::tuple<Set<Ts>*...> sets, std::span<const Entity> entities)
    : m_sets{sets}, m_entities{entities}
{
    skip_incomplete();
}

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
auto View<Ts...>::Iterator::operator*() const -> Element
{
    const Entity entity = m_entities[m_position];
    return std::apply(
        [entity](Set<Ts>*... set) { return Element{entity, *set->try_get(entity)...}; }, m_sets);
}

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
auto View<Ts...>::Iterator::operator++() -> Iterator&
{
    ++m_position;
    skip_incomplete();
    return *this;
}

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
void View<Ts...>::Iterator::operator++(int)
{
    ++*this;
}

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
bool View<Ts...>::Iterator::operator==(std::default_sentinel_t) const
{
    return m_position == m_entities.size();
}

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
void View<Ts...>::Iterator::skip_incomplete()
{
    const auto has_all = [this](Entity entity)
    {
        return std::apply(
            [entity](const Set<Ts>*... set) { return (set->contains(entity) && ...); }, m_sets);
    };
    while (m_position < m_entities.size() && !has_all(m_entities[m_position]))
    {
        ++m_position;
    }
}

} // namespace ember::ecs
