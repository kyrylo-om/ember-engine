export module ember.ecs:view;

import :entity;
import std;

export namespace ember::ecs
{

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
    };

    /// Returns an iterator to the first element.
    Iterator begin();

    /// Returns the sentinel that marks the end of the view.
    std::default_sentinel_t end() const;
};

} // namespace ember::ecs
