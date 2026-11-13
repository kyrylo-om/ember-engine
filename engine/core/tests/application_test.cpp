#include <catch_amalgamated.hpp>

import ember.core;
import ember.ecs;
import std;

using Catch::Matchers::WithinAbs;
using ember::core::Application;
using ember::core::ApplicationConfig;
using ember::core::EventBus;
using ember::core::ExitRequested;
using ember::core::Seconds;
using ember::core::Stage;
using ember::core::Time;
using ember::ecs::World;

namespace
{

constexpr float tolerance = 1e-5f;

// Fixed step that is exact in binary, so step counts in the tests have no rounding error.
constexpr Seconds fixed_step{0.125f};

ApplicationConfig test_config()
{
    return {.fixed_step = fixed_step, .max_frame_time = Seconds{1.0f}, .max_fixed_steps = 4};
}

struct Counter
{
    int value = 0;
};

struct CounterPlugin
{
    int start = 0;

    void build(Application& app) const
    {
        app.world().insert_resource(Counter{start});
        app.add_system(Stage::Update, "count",
                       [](World& world) { ++world.resource<Counter>().value; });
    }
};

struct Message
{
    int value = 0;
};

} // namespace

TEST_CASE("a new application is running and has the time and event bus resources")
{
    const Application app{test_config()};

    CHECK(app.is_running());
    REQUIRE(app.world().try_resource<Time>() != nullptr);
    CHECK(app.world().resource<Time>().fixed_delta == fixed_step);
    CHECK(app.world().resource<Time>().frame == 0);
    CHECK(app.world().try_resource<EventBus>() != nullptr);
}

TEST_CASE("a frame runs the stages in order")
{
    Application app{test_config()};
    std::vector<Stage> order;
    // Added in reverse, so the order comes from the stages and not from the registration.
    for (const Stage stage :
         {Stage::Shutdown, Stage::PostRender, Stage::Render, Stage::PreRender, Stage::PostUpdate,
          Stage::Update, Stage::FixedUpdate, Stage::PreUpdate, Stage::Startup})
    {
        app.add_system(stage, "record", [&order, stage](World&) { order.push_back(stage); });
    }

    app.update(fixed_step);

    CHECK(order == std::vector{Stage::Startup, Stage::PreUpdate, Stage::FixedUpdate, Stage::Update,
                               Stage::PostUpdate, Stage::PreRender, Stage::Render,
                               Stage::PostRender});
}

TEST_CASE("startup runs once, before the first frame")
{
    Application app{test_config()};
    int startup_runs = 0;
    app.add_system(Stage::Startup, "startup", [&startup_runs](World&) { ++startup_runs; });

    app.update(fixed_step);
    app.update(fixed_step);

    CHECK(startup_runs == 1);
}

TEST_CASE("fixed update runs once per fixed step of frame time")
{
    Application app{test_config()};
    int fixed_runs = 0;
    app.add_system(Stage::FixedUpdate, "fixed", [&fixed_runs](World&) { ++fixed_runs; });

    SECTION("a frame shorter than a step runs no fixed update and keeps its time")
    {
        app.update(fixed_step / 2);
        CHECK(fixed_runs == 0);
        CHECK_THAT(app.world().resource<Time>().fixed_alpha, WithinAbs(0.5f, tolerance));

        app.update(fixed_step / 2);
        CHECK(fixed_runs == 1);
    }

    SECTION("a long frame runs several fixed updates")
    {
        app.update(fixed_step * 3);
        CHECK(fixed_runs == 3);
    }

    SECTION("the number of fixed updates per frame is limited")
    {
        app.update(fixed_step * 7);
        CHECK(fixed_runs == 4);
    }
}

TEST_CASE("time is updated before the systems of a frame run")
{
    Application app{test_config()};
    std::vector<Time> seen;
    app.add_system(Stage::Update, "observe",
                   [&seen](World& world) { seen.push_back(world.resource<Time>()); });

    app.update(Seconds{0.25f});
    app.update(Seconds{0.5f});

    REQUIRE(seen.size() == 2);
    CHECK(seen[0].delta == Seconds{0.25f});
    CHECK(seen[0].frame == 0);
    CHECK_THAT(seen[0].elapsed.count(), WithinAbs(0.25, tolerance));
    CHECK(seen[1].delta == Seconds{0.5f});
    CHECK(seen[1].frame == 1);
    CHECK_THAT(seen[1].elapsed.count(), WithinAbs(0.75, tolerance));
    CHECK(seen[1].fixed_delta == fixed_step);
}

TEST_CASE("frame time is limited to the configured range")
{
    Application app{test_config()};

    app.update(Seconds{10.0f});
    CHECK(app.world().resource<Time>().delta == Seconds{1.0f});

    app.update(Seconds{-1.0f});
    CHECK(app.world().resource<Time>().delta == Seconds{0.0f});
}

TEST_CASE("a plugin is built when it is added")
{
    Application app{test_config()};

    app.add_plugin(CounterPlugin{.start = 10}).add_plugin(CounterPlugin{.start = 20});
    REQUIRE(app.world().try_resource<Counter>() != nullptr);
    CHECK(app.world().resource<Counter>().value == 20);

    app.update(fixed_step);
    // Both plugins added a counting system.
    CHECK(app.world().resource<Counter>().value == 22);
    CHECK(app.scheduler().system_count(Stage::Update) == 2);
}

TEST_CASE("an event published in a frame is read in the next frame only")
{
    Application app{test_config()};
    std::vector<std::size_t> read_counts;
    app.add_system(Stage::PreUpdate, "read", [&read_counts](World& world)
                   { read_counts.push_back(world.resource<EventBus>().read<Message>().size()); });
    app.add_system(Stage::Update, "publish",
                   [](World& world)
                   {
                       if (world.resource<Time>().frame == 0)
                       {
                           world.resource<EventBus>().publish(Message{1});
                       }
                   });

    app.update(fixed_step);
    app.update(fixed_step);
    app.update(fixed_step);

    CHECK(read_counts == std::vector<std::size_t>{0, 1, 0});
}

TEST_CASE("an event published during startup is read in the first frame")
{
    Application app{test_config()};
    std::size_t read_count = 0;
    app.add_system(Stage::Startup, "publish",
                   [](World& world) { world.resource<EventBus>().publish(Message{1}); });
    app.add_system(Stage::Update, "read", [&read_count](World& world)
                   { read_count = world.resource<EventBus>().read<Message>().size(); });

    app.update(fixed_step);

    CHECK(read_count == 1);
}

TEST_CASE("an exit request stops the application")
{
    Application app{test_config()};

    SECTION("requested directly")
    {
        app.request_exit();
        CHECK_FALSE(app.is_running());
    }

    SECTION("requested with an event")
    {
        app.world().resource<EventBus>().publish(ExitRequested{});
        CHECK(app.is_running());

        app.update(fixed_step);
        CHECK_FALSE(app.is_running());
    }
}

TEST_CASE("run executes startup, frames until an exit is requested, then shutdown")
{
    Application app{test_config()};
    int startup_runs = 0;
    int frames = 0;
    int shutdown_runs = 0;
    constexpr int frames_to_run = 3;
    app.add_system(Stage::Startup, "startup", [&startup_runs](World&) { ++startup_runs; });
    app.add_system(Stage::Update, "frame",
                   [&frames, &app](World&)
                   {
                       if (++frames == frames_to_run)
                       {
                           app.request_exit();
                       }
                   });
    app.add_system(Stage::Shutdown, "shutdown",
                   [&](World&)
                   {
                       // Shutdown must come after the last frame.
                       CHECK(frames == frames_to_run);
                       ++shutdown_runs;
                   });

    app.run();

    CHECK(startup_runs == 1);
    CHECK(frames == frames_to_run);
    CHECK(shutdown_runs == 1);
    CHECK(app.world().resource<Time>().frame == frames_to_run);
}

TEST_CASE("run executes no frame if an exit is requested during startup")
{
    Application app{test_config()};
    int frames = 0;
    bool did_shutdown_run = false;
    app.add_system(Stage::Startup, "exit", [&app](World&) { app.request_exit(); });
    app.add_system(Stage::Update, "frame", [&frames](World&) { ++frames; });
    app.add_system(Stage::Shutdown, "shutdown",
                   [&did_shutdown_run](World&) { did_shutdown_run = true; });

    app.run();

    CHECK(frames == 0);
    CHECK(did_shutdown_run);
}
