#include <catch_amalgamated.hpp>

import ember.ecs;
import std;

using ember::ecs::Entity;
using ember::ecs::View;
using ember::ecs::World;

namespace
{

struct Position
{
    int x = 0;
};

struct Velocity
{
    int x = 0;
};

struct Frozen
{
};

static_assert(std::ranges::input_range<View<Position, Velocity>>);

// Entities of a view in ascending order, because a view does not promise an order.
template <typename... Ts>
std::vector<Entity> sorted_entities(View<Ts...> view)
{
    std::vector<Entity> entities;
    for (const auto& element : view)
    {
        entities.push_back(std::get<0>(element));
    }
    std::ranges::sort(entities);
    return entities;
}

} // namespace

TEST_CASE("a view visits every entity that has the component")
{
    World world;
    const Entity first = world.create();
    const Entity second = world.create();
    world.create();
    world.add(first, Position{1});
    world.add(second, Position{2});

    int sum = 0;
    for (auto [entity, position] : world.view<Position>())
    {
        CHECK(&position == &world.get<Position>(entity));
        sum += position.x;
    }

    CHECK(sum == 3);
    CHECK(sorted_entities(world.view<Position>()) == std::vector<Entity>{first, second});
}

TEST_CASE("a view skips entities that miss one of the components")
{
    World world;
    const Entity moving = world.create();
    const Entity standing = world.create();
    const Entity ghost = world.create();
    const Entity also_moving = world.create();
    world.add(moving, Position{1});
    world.add(moving, Velocity{1});
    world.add(standing, Position{2});
    world.add(ghost, Velocity{3});
    world.add(also_moving, Position{4});
    world.add(also_moving, Velocity{4});

    const std::vector<Entity> expected{moving, also_moving};
    CHECK(sorted_entities(world.view<Position, Velocity>()) == expected);
    CHECK(sorted_entities(world.view<Velocity, Position>()) == expected);
}

TEST_CASE("changes made through a view are stored")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Position{1});
    world.add(entity, Velocity{10});

    for (auto [viewed, position, velocity] : world.view<Position, Velocity>())
    {
        position.x += velocity.x;
    }

    CHECK(world.get<Position>(entity).x == 11);
}

TEST_CASE("a view is empty if no entity has all the components")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Position{1});

    CHECK(sorted_entities(world.view<Velocity>()).empty());
    CHECK(sorted_entities(world.view<Position, Velocity>()).empty());
    CHECK(sorted_entities(world.view<Velocity, Position>()).empty());

    world.add(entity, Velocity{1});
    world.remove<Velocity>(entity);

    CHECK(sorted_entities(world.view<Position, Velocity>()).empty());
}

TEST_CASE("a view does not visit destroyed entities")
{
    World world;
    const Entity kept = world.create();
    const Entity destroyed = world.create();
    world.add(kept, Position{1});
    world.add(destroyed, Position{2});

    world.destroy(destroyed);

    CHECK(sorted_entities(world.view<Position>()) == std::vector<Entity>{kept});
}

TEST_CASE("a view visits entities with a component without data")
{
    World world;
    const Entity frozen = world.create();
    const Entity free = world.create();
    world.add(frozen, Position{1});
    world.add<Frozen>(frozen);
    world.add(free, Position{2});

    CHECK(sorted_entities(world.view<Position, Frozen>()) == std::vector<Entity>{frozen});
}

TEST_CASE("a view of a const world gives const components")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Position{5});
    const World& const_world = world;

    static_assert(std::same_as<decltype(const_world.view<Position>()), View<const Position>>);
    static_assert(
        std::same_as<std::tuple_element_t<1, View<const Position>::Element>, const Position&>);

    int sum = 0;
    for (auto [viewed, position] : const_world.view<Position>())
    {
        sum += position.x;
    }
    CHECK(sum == 5);
}

TEST_CASE("a view can mix const and changeable components")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Position{1});
    world.add(entity, Velocity{10});

    static_assert(std::same_as<View<Position, const Velocity>::Element,
                               std::tuple<Entity, Position&, const Velocity&>>);

    for (auto [viewed, position, velocity] : world.view<Position, const Velocity>())
    {
        position.x += velocity.x;
    }

    CHECK(world.get<Position>(entity).x == 11);
}
