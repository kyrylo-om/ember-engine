#include <catch_amalgamated.hpp>

import ember.ecs;
import std;

using ember::ecs::Entity;
using ember::ecs::null_entity;
using ember::ecs::World;

namespace
{

struct Position
{
    int x = 0;
    int y = 0;
};

struct Health
{
    int value = 0;
};

struct Settings
{
    int volume = 0;
};

// Resource that writes its id to a log when it is destroyed. The id is a template parameter,
// because a world holds one resource per type.
template <int Id>
class Tracked
{
public:
    explicit Tracked(std::vector<int>& log) : m_log{&log}
    {
    }

    ~Tracked()
    {
        if (m_log != nullptr)
        {
            m_log->push_back(Id);
        }
    }

    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;

    // The object that is moved from no longer writes to the log.
    Tracked(Tracked&& other) noexcept : m_log{std::exchange(other.m_log, nullptr)}
    {
    }

    Tracked& operator=(Tracked&&) = delete;

private:
    std::vector<int>* m_log = nullptr;
};

} // namespace

TEST_CASE("a created entity is alive")
{
    World world;

    const Entity entity = world.create();

    CHECK(world.is_alive(entity));
    CHECK(entity != null_entity);
}

TEST_CASE("created entities are different")
{
    World world;

    const Entity first = world.create();
    const Entity second = world.create();

    CHECK(first != second);
}

TEST_CASE("the null entity is not alive")
{
    World world;
    world.create();

    CHECK_FALSE(world.is_alive(null_entity));
}

TEST_CASE("an entity of an index that was never used is not alive")
{
    World world;

    CHECK_FALSE(world.is_alive(Entity{7, 1}));
}

TEST_CASE("a destroyed entity is not alive")
{
    World world;
    const Entity entity = world.create();

    world.destroy(entity);

    CHECK_FALSE(world.is_alive(entity));
}

TEST_CASE("destroying an entity that is not alive does nothing")
{
    World world;
    const Entity entity = world.create();
    world.destroy(entity);
    const Entity reused = world.create();

    world.destroy(entity);
    world.destroy(null_entity);

    CHECK(world.is_alive(reused));
}

TEST_CASE("a reused index gets a new generation")
{
    World world;
    const Entity old_entity = world.create();
    world.destroy(old_entity);

    const Entity new_entity = world.create();

    REQUIRE(new_entity.index == old_entity.index);
    CHECK(new_entity.generation != old_entity.generation);
    CHECK(world.is_alive(new_entity));
    CHECK_FALSE(world.is_alive(old_entity));
}

TEST_CASE("an added component can be read and changed")
{
    World world;
    const Entity entity = world.create();

    Position& added = world.add(entity, Position{1, 2});
    CHECK(added.x == 1);
    CHECK(&added == &world.get<Position>(entity));

    world.get<Position>(entity).x = 5;
    CHECK(std::as_const(world).get<Position>(entity).x == 5);
    CHECK(world.get<Position>(entity).y == 2);
}

TEST_CASE("a component added without a value has its default values")
{
    World world;
    const Entity entity = world.create();

    world.add<Position>(entity);

    CHECK(world.get<Position>(entity).x == 0);
}

TEST_CASE("adding a component again replaces it")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Health{10});

    world.add(entity, Health{20});

    CHECK(world.get<Health>(entity).value == 20);
}

TEST_CASE("components of different entities and types are independent")
{
    World world;
    const Entity first = world.create();
    const Entity second = world.create();

    world.add(first, Position{1, 1});
    world.add(first, Health{10});
    world.add(second, Position{2, 2});

    CHECK(world.get<Position>(first).x == 1);
    CHECK(world.get<Position>(second).x == 2);
    CHECK(world.get<Health>(first).value == 10);
    CHECK_FALSE(world.has<Health>(second));
}

TEST_CASE("has and try_get report a missing component")
{
    World world;
    const Entity entity = world.create();

    CHECK_FALSE(world.has<Position>(entity));
    CHECK(world.try_get<Position>(entity) == nullptr);
    CHECK(std::as_const(world).try_get<Position>(entity) == nullptr);

    world.add(entity, Position{3, 4});

    CHECK(world.has<Position>(entity));
    REQUIRE(world.try_get<Position>(entity) != nullptr);
    CHECK(world.try_get<Position>(entity)->y == 4);
    CHECK_FALSE(world.has<Health>(entity));
    CHECK(world.try_get<Health>(entity) == nullptr);
}

TEST_CASE("removing a component keeps the components of other entities")
{
    World world;
    const Entity first = world.create();
    const Entity second = world.create();
    const Entity third = world.create();
    world.add(first, Health{1});
    world.add(second, Health{2});
    world.add(third, Health{3});

    world.remove<Health>(second);

    CHECK_FALSE(world.has<Health>(second));
    CHECK(world.is_alive(second));
    CHECK(world.get<Health>(first).value == 1);
    CHECK(world.get<Health>(third).value == 3);
}

TEST_CASE("removing the last remaining component leaves none")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Health{1});

    world.remove<Health>(entity);

    CHECK_FALSE(world.has<Health>(entity));
}

TEST_CASE("removing a missing component does nothing")
{
    World world;
    const Entity entity = world.create();
    world.add(entity, Position{1, 2});

    world.remove<Health>(entity);
    world.remove<Position>(null_entity);

    CHECK(world.has<Position>(entity));
}

TEST_CASE("destroying an entity removes all its components")
{
    World world;
    const Entity entity = world.create();
    const Entity other = world.create();
    world.add(entity, Position{1, 2});
    world.add(entity, Health{10});
    world.add(other, Health{20});

    world.destroy(entity);

    CHECK_FALSE(world.has<Position>(entity));
    CHECK_FALSE(world.has<Health>(entity));
    CHECK(world.try_get<Health>(entity) == nullptr);
    CHECK(world.get<Health>(other).value == 20);
}

TEST_CASE("an entity that reuses an index does not get the old components")
{
    World world;
    const Entity old_entity = world.create();
    world.add(old_entity, Health{10});
    world.destroy(old_entity);

    const Entity new_entity = world.create();
    REQUIRE(new_entity.index == old_entity.index);
    CHECK_FALSE(world.has<Health>(new_entity));

    world.add(new_entity, Health{20});
    CHECK_FALSE(world.has<Health>(old_entity));
    CHECK(world.try_get<Health>(old_entity) == nullptr);
}

TEST_CASE("an inserted resource can be read and changed")
{
    World world;

    Settings& inserted = world.insert_resource(Settings{5});
    CHECK(&inserted == &world.resource<Settings>());

    world.resource<Settings>().volume = 7;
    CHECK(std::as_const(world).resource<Settings>().volume == 7);
}

TEST_CASE("try_resource reports a missing resource")
{
    World world;

    CHECK(world.try_resource<Settings>() == nullptr);
    CHECK(std::as_const(world).try_resource<Settings>() == nullptr);

    world.insert_resource(Settings{5});

    REQUIRE(world.try_resource<Settings>() != nullptr);
    CHECK(std::as_const(world).try_resource<Settings>()->volume == 5);
}

TEST_CASE("inserting a resource again replaces it")
{
    World world;
    world.insert_resource(Settings{5});

    world.insert_resource(Settings{9});

    CHECK(world.resource<Settings>().volume == 9);
}

TEST_CASE("resources are destroyed in reverse order of insertion")
{
    std::vector<int> destroyed;
    {
        World world;
        world.insert_resource(Tracked<1>{destroyed});
        world.insert_resource(Tracked<2>{destroyed});
        world.insert_resource(Tracked<3>{destroyed});
        REQUIRE(destroyed.empty());
    }

    CHECK(destroyed == std::vector<int>{3, 2, 1});
}
