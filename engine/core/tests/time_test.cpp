#include <catch_amalgamated.hpp>

import ember.core;
import std;

using ember::core::FixedTimestep;
using ember::core::Seconds;

// Steps and frame times are powers of two, so the float arithmetic in the tests is exact.

TEST_CASE("a frame shorter than a step runs no steps")
{
    FixedTimestep timestep{Seconds{0.25f}, 4};

    CHECK(timestep.advance(Seconds{0.125f}) == 0);
    CHECK(timestep.alpha() == 0.5f);
}

TEST_CASE("time left over from a frame is added to the next one")
{
    FixedTimestep timestep{Seconds{0.25f}, 4};

    CHECK(timestep.advance(Seconds{0.125f}) == 0);
    CHECK(timestep.advance(Seconds{0.1875f}) == 1);
    CHECK(timestep.alpha() == 0.25f);
}

TEST_CASE("a long frame runs several steps")
{
    FixedTimestep timestep{Seconds{0.25f}, 4};

    CHECK(timestep.advance(Seconds{0.75f}) == 3);
    CHECK(timestep.alpha() == 0.0f);
}

TEST_CASE("time above the step limit is dropped")
{
    FixedTimestep timestep{Seconds{0.25f}, 4};

    CHECK(timestep.advance(Seconds{10.0f}) == 4);
    CHECK(timestep.alpha() == 0.0f);
    CHECK(timestep.advance(Seconds{0.125f}) == 0);
}

TEST_CASE("total simulated time matches the real time")
{
    const Seconds step{1.0f / 60.0f};
    FixedTimestep timestep{step, 5};

    int total_steps = 0;
    for (int frame = 0; frame < 1000; ++frame)
    {
        total_steps += timestep.advance(Seconds{1.0f / 144.0f});
    }

    const Seconds simulated = step * static_cast<float>(total_steps) + step * timestep.alpha();
    CHECK_THAT(simulated.count(), Catch::Matchers::WithinRel(1000.0f / 144.0f, 1e-4f));
}

TEST_CASE("the timestep returns its settings")
{
    const FixedTimestep timestep{Seconds{0.25f}, 4};

    CHECK(timestep.step() == Seconds{0.25f});
    CHECK(timestep.max_steps() == 4);
}
