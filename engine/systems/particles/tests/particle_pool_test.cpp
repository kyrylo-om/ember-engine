#include <catch_amalgamated.hpp>

import ember.common;
import ember.core;
import ember.particles;
import std;

using ember::Vec2;
using ember::core::Seconds;
using ember::particles::Particle;
using ember::particles::ParticlePool;

namespace
{

enum class Simulation
{
    Scalar,
    Simd
};

void simulate(ParticlePool& pool, Simulation simulation, Seconds delta, Vec2 acceleration,
              Seconds lifetime)
{
    if (simulation == Simulation::Simd)
    {
        pool.simulate_simd(delta, acceleration, lifetime);
    }
    else
    {
        pool.simulate_scalar(delta, acceleration, lifetime);
    }
}

} // namespace

// Times and velocities are powers of two, so the float arithmetic in the tests is exact.

TEST_CASE("a new pool is empty")
{
    const ParticlePool pool{8};

    CHECK(pool.size() == 0);
    CHECK(pool.capacity() == 8);
}

TEST_CASE("particles are emitted until the pool is full")
{
    ParticlePool pool{2};

    CHECK(pool.emit({1.0f, 2.0f}, {3.0f, 4.0f}));
    CHECK(pool.emit({5.0f, 6.0f}, {7.0f, 8.0f}));
    CHECK_FALSE(pool.emit({9.0f, 9.0f}, {9.0f, 9.0f}));

    REQUIRE(pool.size() == 2);
    const Particle second = pool.particle(1);
    CHECK(second.position == Vec2{5.0f, 6.0f});
    CHECK(second.velocity == Vec2{7.0f, 8.0f});
    CHECK(second.age == Seconds{0.0f});
}

TEST_CASE("a particle accelerates, moves by its new velocity and ages")
{
    const Simulation simulation = GENERATE(Simulation::Scalar, Simulation::Simd);
    ParticlePool pool{4};
    REQUIRE(pool.emit({1.0f, 1.0f}, {4.0f, 0.0f}));

    simulate(pool, simulation, Seconds{0.5f}, {0.0f, 2.0f}, Seconds{10.0f});

    const Particle particle = pool.particle(0);
    CHECK(particle.velocity == Vec2{4.0f, 1.0f});
    CHECK(particle.position == Vec2{3.0f, 1.5f});
    CHECK(particle.age == Seconds{0.5f});
}

TEST_CASE("particles are removed when their age reaches the lifetime")
{
    const Simulation simulation = GENERATE(Simulation::Scalar, Simulation::Simd);
    ParticlePool pool{4};
    REQUIRE(pool.emit({}, {}));

    simulate(pool, simulation, Seconds{0.5f}, {}, Seconds{1.0f});
    CHECK(pool.size() == 1);

    simulate(pool, simulation, Seconds{0.5f}, {}, Seconds{1.0f});
    CHECK(pool.size() == 0);
}

TEST_CASE("removing a particle keeps the other particles")
{
    const Simulation simulation = GENERATE(Simulation::Scalar, Simulation::Simd);
    ParticlePool pool{4};

    // Emitted at different times, so the oldest one expires first.
    REQUIRE(pool.emit({}, {1.0f, 0.0f}));
    simulate(pool, simulation, Seconds{0.5f}, {}, Seconds{1.0f});
    REQUIRE(pool.emit({}, {2.0f, 0.0f}));
    REQUIRE(pool.emit({}, {3.0f, 0.0f}));
    simulate(pool, simulation, Seconds{0.5f}, {}, Seconds{1.0f});

    REQUIRE(pool.size() == 2);
    std::vector<float> velocities{pool.particle(0).velocity.x, pool.particle(1).velocity.x};
    std::ranges::sort(velocities);
    CHECK(velocities == std::vector{2.0f, 3.0f});
}

TEST_CASE("a pool larger than one simd vector is simulated completely")
{
    constexpr std::size_t particle_count = 37;
    ParticlePool pool{particle_count};
    for (std::size_t index = 0; index < particle_count; ++index)
    {
        REQUIRE(pool.emit({}, {1.0f, 0.0f}));
    }

    pool.simulate_simd(Seconds{0.5f}, {}, Seconds{10.0f});

    for (std::size_t index = 0; index < particle_count; ++index)
    {
        CHECK(pool.particle(index).position == Vec2{0.5f, 0.0f});
    }
}

TEST_CASE("simd and scalar simulation give the same results")
{
    constexpr std::size_t particle_count = 1003;
    constexpr int step_count = 120;
    const Vec2 gravity{0.0f, 9.81f};
    const Seconds lifetime{1.5f};
    const Seconds delta{1.0f / 60.0f};

    ParticlePool scalar_pool{particle_count};
    ParticlePool simd_pool{particle_count};
    std::mt19937 random{42};
    std::uniform_real_distribution<float> coordinate{-100.0f, 100.0f};

    for (int step = 0; step < step_count; ++step)
    {
        // A few particles per step, so their ages differ and they expire at different steps.
        for (int index = 0; index < 10; ++index)
        {
            const Vec2 position{coordinate(random), coordinate(random)};
            const Vec2 velocity{coordinate(random), coordinate(random)};
            CHECK(scalar_pool.emit(position, velocity) == simd_pool.emit(position, velocity));
        }
        scalar_pool.simulate_scalar(delta, gravity, lifetime);
        simd_pool.simulate_simd(delta, gravity, lifetime);
    }

    REQUIRE(scalar_pool.size() > 0);
    REQUIRE(simd_pool.size() == scalar_pool.size());
    for (std::size_t index = 0; index < scalar_pool.size(); ++index)
    {
        const Particle expected = scalar_pool.particle(index);
        const Particle actual = simd_pool.particle(index);
        CHECK(actual.position == expected.position);
        CHECK(actual.velocity == expected.velocity);
        CHECK(actual.age == expected.age);
    }
}

TEST_CASE("clear removes all particles")
{
    ParticlePool pool{4};
    REQUIRE(pool.emit({}, {}));

    pool.clear();

    CHECK(pool.size() == 0);
    CHECK(pool.emit({}, {}));
}
