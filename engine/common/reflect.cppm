export module ember.common:reflect;

import std;

export namespace ember
{

/// Returns the name of a type without its namespaces.
/// @tparam T Type whose name is returned.
/// @return Name of the type, for example "Transform" for ember::Transform.
template <typename T>
consteval std::string_view type_name();

/// Calls a function for each non-static data member of an aggregate, in declaration order.
/// @tparam T Aggregate type. If it is const, the members are passed as const references.
/// @param object Object whose members are visited.
/// @param visitor Called as visitor(name, member) with the name of the member as
/// std::string_view and a reference to the member.
template <typename T, typename F>
    requires std::is_aggregate_v<std::remove_const_t<T>>
constexpr void for_each_field(T& object, F&& visitor);

/// Returns the name of an enumerator.
/// @tparam E Enumeration type.
/// @param value Value to look up.
/// @return Name of the enumerator, for example "FixedUpdate" for Stage::FixedUpdate, or
/// nothing if no enumerator has this value.
template <typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr std::optional<std::string_view> enum_to_string(E value);

/// Returns the enumerator with the given name.
/// @tparam E Enumeration type.
/// @param name Name of the enumerator without the type, for example "FixedUpdate".
/// @return The enumerator, or nothing if E has no enumerator with this name.
template <typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr std::optional<E> enum_from_string(std::string_view name);

} // namespace ember
