#include <catch_amalgamated.hpp>

import ember.common;
import ember.core;
import ember.ecs;
import ember.particles;
import std;

using Catch::Matchers::WithinAbs;
using ember::Vec2;
using ember::core::Seconds;
using ember::ecs::Emitter;
using ember::particles::Particle;
using ember::particles::ParticlePool;
using ember::particles::ParticleSpawner;

namespace
{

constexpr std::uint32_t seed = 7;
// Room for numerical error of cos and sin in the checks of speeds and angles.
constexpr float tolerance = 1e-3f;

Emitter burst_only(std::uint32_t count)
{
    Emitter emitter{};
    emitter.is_emitting = false;
    emitter.burst_count = count;
    return emitter;
}

} // namespace

TEST_CASE("a burst emits all its particles at once and is reset")
{
    ParticleSpawner spawner{seed};
    ParticlePool pool{16};
    Emitter emitter = burst_only(5);

    CHECK(spawner.spawn(emitter, {}, Seconds{0.0f}, pool) == 5);
    CHECK(pool.size() == 5);
    CHECK(emitter.burst_count == 0);

    CHECK(spawner.spawn(emitter, {}, Seconds{0.0f}, pool) == 0);
}

TEST_CASE("continuous emission carries fractions of a particle to the next update")
{
    ParticleSpawner spawner{seed};
    ParticlePool pool{16};
    Emitter emitter{};
    emitter.emission_rate = 10.0f;

    // 2.5 particles are due in each update: 2, then 2.5 + 0.5 = 3.
    CHECK(spawner.spawn(emitter, {}, Seconds{0.25f}, pool) == 2);
    CHECK(spawner.spawn(emitter, {}, Seconds{0.25f}, pool) == 3);
}

TEST_CASE("an emitter that is not emitting emits only bursts")
{
    ParticleSpawner spawner{seed};
    ParticlePool pool{16};
    Emitter emitter = burst_only(1);
    emitter.emission_rate = 100.0f;

    CHECK(spawner.spawn(emitter, {}, Seconds{1.0f}, pool) == 1);
}

TEST_CASE("a fraction left from before does not add a particle when emission restarts")
{
    ParticleSpawner spawner{seed};
    ParticlePool pool{16};
    Emitter emitter{};
    emitter.emission_rate = 1.0f;

    CHECK(spawner.spawn(emitter, {}, Seconds{0.75f}, pool) == 0);
    emitter.is_emitting = false;
    CHECK(spawner.spawn(emitter, {}, Seconds{1.0f}, pool) == 0);
    emitter.is_emitting = true;
    CHECK(spawner.spawn(emitter, {}, Seconds{0.5f}, pool) == 0);
}

TEST_CASE("particles that do not fit into the pool are dropped")
{
    ParticleSpawner spawner{seed};
    ParticlePool pool{3};
    Emitter emitter = burst_only(10);

    CHECK(spawner.spawn(emitter, {}, Seconds{0.0f}, pool) == 3);
    CHECK(pool.size() == 3);
    CHECK(emitter.burst_count == 0);
}

TEST_CASE("particles start at the origin with a speed and direction in range")
{
    ParticleSpawner spawner{seed};
    ParticlePool pool{1000};
    Emitter emitter = burst_only(1000);
    emitter.min_speed = 50.0f;
    emitter.max_speed = 100.0f;
    emitter.direction = 0.5f;
    emitter.spread = 0.25f;
    const Vec2 origin{10.0f, 20.0f};

    REQUIRE(spawner.spawn(emitter, origin, Seconds{0.0f}, pool) == 1000);

    for (std::size_t index = 0; index < pool.size(); ++index)
    {
        const Particle particle = pool.particle(index);
        CHECK(particle.position == origin);
        CHECK(particle.age == Seconds{0.0f});

        const float speed = particle.velocity.length();
        CHECK(speed >= emitter.min_speed - tolerance);
        CHECK(speed <= emitter.max_speed + tolerance);

        const float angle = std::atan2(particle.velocity.y, particle.velocity.x);
        CHECK(angle >= emitter.direction - emitter.spread - tolerance);
        CHECK(angle <= emitter.direction + emitter.spread + tolerance);
    }
}

TEST_CASE("a spread of zero emits every particle in the given direction")
{
    ParticleSpawner spawner{seed};
    ParticlePool pool{10};
    Emitter emitter = burst_only(10);
    emitter.min_speed = 2.0f;
    emitter.max_speed = 2.0f;
    emitter.direction = std::numbers::pi_v<float> / 2.0f;
    emitter.spread = 0.0f;

    REQUIRE(spawner.spawn(emitter, {}, Seconds{0.0f}, pool) == 10);

    for (std::size_t index = 0; index < pool.size(); ++index)
    {
        // Straight down, because y points down.
        CHECK_THAT(pool.particle(index).velocity.x, WithinAbs(0.0f, tolerance));
        CHECK_THAT(pool.particle(index).velocity.y, WithinAbs(2.0f, tolerance));
    }
}

TEST_CASE("the same seed emits the same particles")
{
    ParticleSpawner first_spawner{seed};
    ParticleSpawner second_spawner{seed};
    ParticlePool first_pool{20};
    ParticlePool second_pool{20};
    Emitter first_emitter = burst_only(20);
    Emitter second_emitter = burst_only(20);

    REQUIRE(first_spawner.spawn(first_emitter, {}, Seconds{0.0f}, first_pool) == 20);
    REQUIRE(second_spawner.spawn(second_emitter, {}, Seconds{0.0f}, second_pool) == 20);

    for (std::size_t index = 0; index < first_pool.size(); ++index)
    {
        CHECK(first_pool.particle(index).velocity == second_pool.particle(index).velocity);
    }
}

TEST_CASE("swapped or negative ranges are corrected")
{
    ParticleSpawner spawner{seed};
    ParticlePool pool{100};
    Emitter emitter = burst_only(100);
    emitter.is_emitting = true;
    emitter.emission_rate = -10.0f;
    emitter.min_speed = 100.0f;
    emitter.max_speed = 50.0f;
    emitter.direction = 0.0f;
    emitter.spread = -0.25f;

    REQUIRE(spawner.spawn(emitter, {}, Seconds{1.0f}, pool) == 100);

    for (std::size_t index = 0; index < pool.size(); ++index)
    {
        const Vec2 velocity = pool.particle(index).velocity;
        CHECK(velocity.length() >= 50.0f - tolerance);
        CHECK(velocity.length() <= 100.0f + tolerance);
        CHECK(std::abs(std::atan2(velocity.y, velocity.x)) <= 0.25f + tolerance);
    }
}
