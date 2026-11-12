#include <catch_amalgamated.hpp>

import ember.ecs;
import std;

using ember::ecs::Commands;
using ember::ecs::Entity;
using ember::ecs::World;

namespace
{

struct Health
{
    int value = 0;
};

struct Poisoned
{
};

} // namespace

TEST_CASE("recorded commands change nothing until they are applied")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Health{10});
    Commands& commands = world.commands();

    commands.add(entity, Health{20});
    commands.add<Poisoned>(entity);
    commands.destroy(entity);

    CHECK(world.is_alive(entity));
    CHECK(world.get<Health>(entity).value == 10);
    CHECK_FALSE(world.has<Poisoned>(entity));

    world.apply_commands();

    CHECK_FALSE(world.is_alive(entity));
}

TEST_CASE("a recorded add replaces the component")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Health{10});

    world.commands().add(entity, Health{20});
    world.apply_commands();

    CHECK(world.get<Health>(entity).value == 20);
}

TEST_CASE("a recorded remove removes the component")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Health{10});

    world.commands().remove<Health>(entity);
    world.apply_commands();

    CHECK_FALSE(world.has<Health>(entity));
    CHECK(world.is_alive(entity));
}

TEST_CASE("commands are applied in recording order")
{
    World world;
    const Entity entity = world.create();
    Commands& commands = world.commands();

    commands.add(entity, Health{1});
    commands.remove<Health>(entity);
    commands.add(entity, Health{2});
    commands.add(entity, Health{3});
    world.apply_commands();

    CHECK(world.get<Health>(entity).value == 3);

    commands.add<Poisoned>(entity);
    commands.remove<Poisoned>(entity);
    world.apply_commands();

    CHECK_FALSE(world.has<Poisoned>(entity));
}

TEST_CASE("an entity created by a command is alive after the commands are applied")
{
    World world;
    Commands& commands = world.commands();

    const Entity entity = commands.create();
    commands.add(entity, Health{5});

    CHECK_FALSE(world.is_alive(entity));
    CHECK_FALSE(world.has<Health>(entity));

    world.apply_commands();

    CHECK(world.is_alive(entity));
    CHECK(world.get<Health>(entity).value == 5);
}

TEST_CASE("entities created by commands and by the world are different")
{
    World world;
    Commands& commands = world.commands();

    const Entity first = commands.create();
    const Entity direct = world.create();
    const Entity second = commands.create();
    world.apply_commands();

    CHECK(first != direct);
    CHECK(first != second);
    CHECK(direct != second);
    CHECK(world.is_alive(first));
    CHECK(world.is_alive(direct));
    CHECK(world.is_alive(second));
}

TEST_CASE("an entity can be created and destroyed by the same commands")
{
    World world;
    Commands& commands = world.commands();

    const Entity entity = commands.create();
    commands.add(entity, Health{5});
    commands.destroy(entity);
    world.apply_commands();

    CHECK_FALSE(world.is_alive(entity));
    CHECK_FALSE(world.has<Health>(entity));
}

TEST_CASE("commands for an entity that is not alive do nothing")
{
    World world;
    const Entity entity = world.create();
    Commands& commands = world.commands();

    commands.add(entity, Health{5});
    commands.remove<Health>(entity);
    commands.destroy(entity);
    world.destroy(entity);
    const Entity reused = world.create();
    world.apply_commands();

    REQUIRE(reused.index == entity.index);
    CHECK(world.is_alive(reused));
    CHECK_FALSE(world.has<Health>(reused));
}

TEST_CASE("applied commands are not applied again")
{
    World world;
    const Entity entity = world.create();

    world.commands().add(entity, Health{5});
    world.apply_commands();
    world.get<Health>(entity).value = 9;
    world.apply_commands();

    CHECK(world.get<Health>(entity).value == 9);
}

TEST_CASE("commands can change entities while a view is iterated")
{
    World world;
    std::vector<Entity> entities;
    for (int value = 0; value < 6; ++value)
    {
        entities.push_back(world.create());
        world.add(entities.back(), Health{value});
    }

    int visited = 0;
    for (auto [entity, health] : world.view<Health>())
    {
        ++visited;
        if (health.value % 2 == 0)
        {
            world.commands().destroy(entity);
        }
        else
        {
            world.commands().add<Poisoned>(entity);
            world.commands().add(world.commands().create(), Health{100});
        }
    }
    world.apply_commands();

    CHECK(visited == 6);
    int alive = 0;
    int poisoned = 0;
    for (auto [entity, health] : world.view<Health>())
    {
        ++alive;
        poisoned += world.has<Poisoned>(entity) ? 1 : 0;
    }
    CHECK(alive == 6);
    CHECK(poisoned == 3);
    CHECK_FALSE(world.is_alive(entities[0]));
    CHECK(world.is_alive(entities[1]));
}
