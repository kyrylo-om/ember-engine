#include <catch_amalgamated.hpp>

import ember.core;
import ember.ecs;
import std;

using ember::core::Scheduler;
using ember::core::Stage;
using ember::ecs::Entity;
using ember::ecs::World;

namespace
{

struct Marker
{
};

} // namespace

TEST_CASE("a new scheduler has no systems")
{
    const Scheduler scheduler;

    CHECK(scheduler.system_count(Stage::Startup) == 0);
    CHECK(scheduler.system_count(Stage::Update) == 0);
    CHECK(scheduler.system_count(Stage::Shutdown) == 0);
}

TEST_CASE("systems are counted per stage")
{
    Scheduler scheduler;
    scheduler.add_system(Stage::Update, "first", [](World&) {});
    scheduler.add_system(Stage::Update, "second", [](World&) {});
    scheduler.add_system(Stage::Render, "third", [](World&) {});

    CHECK(scheduler.system_count(Stage::Update) == 2);
    CHECK(scheduler.system_count(Stage::Render) == 1);
    CHECK(scheduler.system_count(Stage::PreUpdate) == 0);
}

TEST_CASE("systems of a stage run in the order they were added")
{
    Scheduler scheduler;
    World world;
    std::vector<int> order;
    scheduler.add_system(Stage::Update, "first", [&order](World&) { order.push_back(1); });
    scheduler.add_system(Stage::Update, "second", [&order](World&) { order.push_back(2); });
    scheduler.add_system(Stage::Update, "third", [&order](World&) { order.push_back(3); });

    scheduler.run_stage(Stage::Update, world);

    CHECK(order == std::vector{1, 2, 3});
}

TEST_CASE("running a stage runs only the systems of that stage")
{
    Scheduler scheduler;
    World world;
    int update_runs = 0;
    int render_runs = 0;
    scheduler.add_system(Stage::Update, "update", [&update_runs](World&) { ++update_runs; });
    scheduler.add_system(Stage::Render, "render", [&render_runs](World&) { ++render_runs; });

    scheduler.run_stage(Stage::Update, world);
    scheduler.run_stage(Stage::Update, world);

    CHECK(update_runs == 2);
    CHECK(render_runs == 0);
}

TEST_CASE("running a stage without systems does nothing")
{
    Scheduler scheduler;
    World world;

    CHECK_NOTHROW(scheduler.run_stage(Stage::FixedUpdate, world));
}

TEST_CASE("a system gets the world passed to the stage")
{
    Scheduler scheduler;
    World world;
    const World* seen = nullptr;
    scheduler.add_system(Stage::Update, "observer", [&seen](World& passed) { seen = &passed; });

    scheduler.run_stage(Stage::Update, world);

    CHECK(seen == &world);
}

TEST_CASE("deferred commands are applied after all systems of the stage")
{
    Scheduler scheduler;
    World world;
    Entity created{};
    bool was_alive_during_stage = true;
    scheduler.add_system(Stage::Update, "creator",
                         [&created](World& current) { created = current.commands().create(); });
    scheduler.add_system(Stage::Update, "observer", [&](World& current)
                         { was_alive_during_stage = current.is_alive(created); });

    scheduler.run_stage(Stage::Update, world);

    CHECK_FALSE(was_alive_during_stage);
    CHECK(world.is_alive(created));
}

TEST_CASE("the next stage sees the commands of the previous stage")
{
    Scheduler scheduler;
    World world;
    const Entity entity = world.create();
    bool has_marker = false;
    scheduler.add_system(Stage::Update, "marker",
                         [entity](World& current) { current.commands().add<Marker>(entity); });
    scheduler.add_system(Stage::PostUpdate, "observer", [&has_marker, entity](World& current)
                         { has_marker = current.has<Marker>(entity); });

    scheduler.run_stage(Stage::Update, world);
    scheduler.run_stage(Stage::PostUpdate, world);

    CHECK(has_marker);
}

TEST_CASE("a system can own a resource that cannot be copied")
{
    Scheduler scheduler;
    World world;
    int result = 0;
    scheduler.add_system(Stage::Update, "owner",
                         [value = std::make_unique<int>(5), &result](World&) { result = *value; });

    scheduler.run_stage(Stage::Update, world);

    CHECK(result == 5);
}

TEST_CASE("an exception of a system is logged, stops the stage and is thrown further")
{
    Scheduler scheduler;
    World world;
    Entity created{};
    bool did_later_system_run = false;
    scheduler.add_system(Stage::Update, "creator",
                         [&created](World& current) { created = current.commands().create(); });
    scheduler.add_system(Stage::Update, "failing",
                         [](World&) { throw std::runtime_error{"failure"}; });
    scheduler.add_system(Stage::Update, "later",
                         [&did_later_system_run](World&) { did_later_system_run = true; });

    std::vector<std::string> errors;
    const ember::core::LogSinkId sink = ember::core::add_log_sink(
        [&errors](ember::core::LogLevel level, std::string_view message)
        {
            if (level == ember::core::LogLevel::Error)
            {
                errors.emplace_back(message);
            }
        });
    CHECK_THROWS_AS(scheduler.run_stage(Stage::Update, world), std::runtime_error);
    ember::core::remove_log_sink(sink);

    REQUIRE(errors.size() == 1);
    CHECK(errors.front() == "System 'failing' failed: failure");
    CHECK_FALSE(did_later_system_run);
    CHECK_FALSE(world.is_alive(created));
}
