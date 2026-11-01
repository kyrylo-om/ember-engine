#include <catch_amalgamated.hpp>

import ember.core;
import std;

using ember::core::EventBus;

namespace
{

struct Hit
{
    int damage = 0;
};

struct Spawn
{
    int id = 0;
};

// Identifies the thread that published an event and its position in that thread's sequence.
struct Numbered
{
    int thread = 0;
    int number = 0;
};

} // namespace

TEST_CASE("published events can be read only in the next frame")
{
    EventBus bus;

    bus.publish(Hit{10});
    CHECK(bus.read<Hit>().empty());

    bus.advance_frame();
    REQUIRE(bus.read<Hit>().size() == 1);
    CHECK(bus.read<Hit>()[0].damage == 10);
}

TEST_CASE("events are delivered in publishing order")
{
    EventBus bus;

    bus.publish(Hit{1});
    bus.publish(Hit{2});
    bus.publish(Hit{3});
    bus.advance_frame();

    const std::span<const Hit> hits = bus.read<Hit>();
    REQUIRE(hits.size() == 3);
    CHECK(hits[0].damage == 1);
    CHECK(hits[1].damage == 2);
    CHECK(hits[2].damage == 3);
}

TEST_CASE("events are dropped after the frame in which they are readable")
{
    EventBus bus;

    bus.publish(Hit{10});
    bus.advance_frame();
    bus.advance_frame();

    CHECK(bus.read<Hit>().empty());
}

TEST_CASE("events published while others are readable wait for the next frame")
{
    EventBus bus;

    bus.publish(Hit{1});
    bus.advance_frame();
    bus.publish(Hit{2});

    REQUIRE(bus.read<Hit>().size() == 1);
    CHECK(bus.read<Hit>()[0].damage == 1);

    bus.advance_frame();
    REQUIRE(bus.read<Hit>().size() == 1);
    CHECK(bus.read<Hit>()[0].damage == 2);
}

TEST_CASE("reading does not consume events")
{
    EventBus bus;

    bus.publish(Hit{10});
    bus.advance_frame();

    CHECK(bus.read<Hit>().size() == 1);
    CHECK(bus.read<Hit>().size() == 1);
}

TEST_CASE("events of different types are kept apart")
{
    EventBus bus;

    bus.publish(Hit{10});
    bus.publish(Spawn{7});
    bus.publish(Spawn{8});
    bus.advance_frame();

    CHECK(bus.read<Hit>().size() == 1);
    REQUIRE(bus.read<Spawn>().size() == 2);
    CHECK(bus.read<Spawn>()[1].id == 8);
}

TEST_CASE("reading a type that was never published returns no events")
{
    const EventBus bus;

    CHECK(bus.read<Hit>().empty());
}

TEST_CASE("const event types use the same events as their plain types")
{
    EventBus bus;

    bus.publish<const Hit>(Hit{10});
    bus.advance_frame();

    CHECK(bus.read<Hit>().size() == 1);
    CHECK(bus.read<const Hit>().size() == 1);
}

TEST_CASE("a moved bus keeps its events")
{
    EventBus bus;
    bus.publish(Hit{10});
    bus.advance_frame();

    const EventBus moved{std::move(bus)};
    REQUIRE(moved.read<Hit>().size() == 1);
    CHECK(moved.read<Hit>()[0].damage == 10);

    EventBus assigned;
    assigned = EventBus{};
    CHECK(assigned.read<Hit>().empty());
}

TEST_CASE("events published from several threads are all delivered in per-thread order")
{
    constexpr int thread_count = 4;
    constexpr int events_per_thread = 1000;
    EventBus bus;

    {
        std::vector<std::jthread> threads;
        for (int thread = 0; thread < thread_count; ++thread)
        {
            threads.emplace_back(
                [&bus, thread]
                {
                    for (int number = 0; number < events_per_thread; ++number)
                    {
                        bus.publish(Numbered{thread, number});
                        // Reading while other threads publish is allowed as well.
                        static_cast<void>(bus.read<Numbered>());
                    }
                });
        }
    }
    bus.advance_frame();

    const std::span<const Numbered> events = bus.read<Numbered>();
    REQUIRE(events.size() == static_cast<std::size_t>(thread_count * events_per_thread));

    std::array<int, thread_count> next_numbers{};
    for (const Numbered& event : events)
    {
        CHECK(event.number == next_numbers[static_cast<std::size_t>(event.thread)]);
        ++next_numbers[static_cast<std::size_t>(event.thread)];
    }
}
