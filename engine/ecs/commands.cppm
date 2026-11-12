export module ember.ecs:commands;

import :entity;
import std;

export namespace ember::ecs
{

class World;

/// Records changes to entities and components and applies them to the world later.
///
/// A system that iterates a view must not create or destroy entities or add or remove
/// components directly. It records these changes here, and the world applies them in
/// recording order in `World::apply_commands()`. The scheduler calls it after every stage.
class Commands
{
public:
    ~Commands();

    Commands(const Commands&) = delete;
    Commands& operator=(const Commands&) = delete;
    Commands(Commands&&) = delete;
    Commands& operator=(Commands&&) = delete;

    /// Records creating an entity without components.
    /// @return The entity, reserved at once so later commands can refer to it. It becomes
    /// alive when the commands are applied.
    Entity create();

    /// Records destroying an entity and all its components.
    /// @param entity Entity to destroy. Nothing happens if it is not alive when the
    /// commands are applied.
    void destroy(Entity entity);

    /// Records adding a component to an entity, replacing the component it already has.
    /// @tparam T Type of the component.
    /// @param entity Entity that gets the component. Nothing happens if it is not alive when
    /// the commands are applied.
    /// @param component The component to add.
    template <typename T>
    void add(Entity entity, T component = {});

    /// Records removing a component from an entity.
    /// @tparam T Type of the component.
    /// @param entity Entity that loses the component. Nothing happens if it is not alive or
    /// has no such component when the commands are applied.
    template <typename T>
    void remove(Entity entity);

private:
    friend class World;

    /// Creates an empty command list that reserves entities in the given world.
    explicit Commands(World& world);

    World* m_world = nullptr;
    // Recorded changes in recording order.
    std::vector<std::move_only_function<void(World&)>> m_operations{};
};

} // namespace ember::ecs
