#include <catch_amalgamated.hpp>

import ember.core;
import ember.ecs;
import std;

using ember::core::Application;
using ember::core::EventBus;
using ember::core::FixedTimestep;
using ember::core::Scheduler;
using ember::core::Seconds;
using ember::core::Stage;
using ember::ecs::World;

namespace
{

constexpr Seconds frame{1.0f / 60.0f};

struct Collision
{
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    float depth = 0.0f;
};

} // namespace

TEST_CASE("publishing and reading events")
{
    constexpr std::uint32_t event_count = 10'000;

    EventBus bus;
    BENCHMARK("publish 10000 events, advance the frame, read them")
    {
        for (std::uint32_t index = 0; index < event_count; ++index)
        {
            bus.publish(Collision{index, index + 1, 0.5f});
        }
        bus.advance_frame();

        float total_depth = 0.0f;
        for (const Collision& collision : bus.read<Collision>())
        {
            total_depth += collision.depth;
        }
        return total_depth;
    };

    // One event at a time, which is how most systems publish: the cost of one call.
    BENCHMARK("publish one event and advance the frame")
    {
        bus.publish(Collision{1, 2, 0.5f});
        bus.advance_frame();
        return bus.read<Collision>().size();
    };
}

TEST_CASE("running systems")
{
    // Systems that do almost nothing, so the time is the cost of the scheduler itself.
    for (const int system_count : {1, 10, 100})
    {
        Scheduler scheduler;
        World world;
        std::uint64_t calls = 0;
        for (int index = 0; index < system_count; ++index)
        {
            scheduler.add_system(Stage::Update, "count", [&calls](World&) { ++calls; });
        }

        BENCHMARK(std::format("a stage with {} systems", system_count))
        {
            scheduler.run_stage(Stage::Update, world);
            return calls;
        };
    }
}

TEST_CASE("a frame of an application without systems")
{
    Application app;
    BENCHMARK("update")
    {
        app.update(frame);
        return app.is_running();
    };
}

TEST_CASE("splitting frame time into fixed steps")
{
    FixedTimestep timestep{frame, 5};
    BENCHMARK("advance")
    {
        return timestep.advance(frame);
    };
}

TEST_CASE("logging a message that the level filters out")
{
    // A debug message is below the default level. It must cost almost nothing, because such
    // calls stay in the code of a finished game.
    const int value = 42;
    BENCHMARK("log_debug below the level")
    {
        ember::core::log_debug("value {} of {}", value, "something");
        return value;
    };
}
