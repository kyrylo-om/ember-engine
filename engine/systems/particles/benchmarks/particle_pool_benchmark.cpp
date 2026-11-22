#include <catch_amalgamated.hpp>

import ember.common;
import ember.core;
import ember.ecs;
import ember.particles;
import std;

using ember::Vec2;
using ember::core::Application;
using ember::core::Seconds;
using ember::ecs::Emitter;
using ember::ecs::Entity;
using ember::ecs::Transform;
using ember::particles::ParticlePlugin;
using ember::particles::ParticlePool;
using ember::particles::ParticleSpawner;

namespace
{

constexpr Seconds frame{1.0f / 60.0f};
constexpr Vec2 gravity{0.0f, 98.0f};
// Long enough that no particle is removed while a benchmark runs, so every run of a
// benchmark simulates the same number of particles.
constexpr Seconds endless{1.0e9f};

// From a pool that fits into the first cache of the processor to one much larger than all
// of its caches.
constexpr std::array<std::size_t, 4> particle_counts{1'000, 10'000, 100'000, 1'000'000};

ParticlePool full_pool(std::size_t particle_count)
{
    ParticlePool pool{particle_count};
    std::minstd_rand random{1};
    std::uniform_real_distribution<float> coordinate{-100.0f, 100.0f};
    for (std::size_t index = 0; index < particle_count; ++index)
    {
        pool.emit({coordinate(random), coordinate(random)},
                  {coordinate(random), coordinate(random)});
    }
    return pool;
}

Emitter endless_emitter(float emission_rate)
{
    Emitter emitter{};
    emitter.emission_rate = emission_rate;
    emitter.lifetime = Seconds{1.0f};
    emitter.acceleration = gravity;
    return emitter;
}

} // namespace

TEST_CASE("simulating a full pool with simd and one particle at a time")
{
    for (const std::size_t particle_count : particle_counts)
    {
        ParticlePool simd_pool = full_pool(particle_count);
        BENCHMARK(std::format("simd, {} particles", particle_count))
        {
            simd_pool.simulate_simd(frame, gravity, endless);
            return simd_pool.size();
        };

        ParticlePool scalar_pool = full_pool(particle_count);
        BENCHMARK(std::format("scalar, {} particles", particle_count))
        {
            scalar_pool.simulate_scalar(frame, gravity, endless);
            return scalar_pool.size();
        };
    }
}

TEST_CASE("simulating a pool in which every particle is removed")
{
    constexpr std::size_t particle_count = 100'000;

    // Removing is the slow path of the simulation: every removed particle is replaced by the
    // last one. Filling the pool is part of the measured time in both benchmarks.
    BENCHMARK("fill only")
    {
        return full_pool(particle_count).size();
    };
    BENCHMARK("fill, then remove all")
    {
        ParticlePool pool = full_pool(particle_count);
        pool.simulate_simd(frame, gravity, Seconds::zero());
        return pool.size();
    };
}

TEST_CASE("emitting particles")
{
    constexpr std::size_t particle_count = 100'000;

    BENCHMARK("a burst of 100000 particles")
    {
        ParticlePool pool{particle_count};
        ParticleSpawner spawner{1};
        Emitter emitter = endless_emitter(0.0f);
        emitter.burst_count = static_cast<std::uint32_t>(particle_count);
        return spawner.spawn(emitter, {}, frame, pool);
    };
}

TEST_CASE("a frame of the particle plugin")
{
    // Every emitter keeps its pool full: it emits as many particles per second as fit into
    // the pool, and each lives one second.
    constexpr std::size_t particles_per_emitter = 1024;
    constexpr int warm_up_frames = 90;

    for (const int emitter_count : {10, 100, 1000})
    {
        for (const bool use_simd : {true, false})
        {
            Application app;
            app.add_plugin(
                ParticlePlugin{.settings = {.max_particles_per_emitter = particles_per_emitter,
                                            .use_simd = use_simd}});
            for (int index = 0; index < emitter_count; ++index)
            {
                const Entity entity = app.world().create();
                app.world().add<Transform>(entity);
                app.world().add<Emitter>(
                    entity, endless_emitter(static_cast<float>(particles_per_emitter)));
            }
            // The pools fill up during the first second.
            for (int index = 0; index < warm_up_frames; ++index)
            {
                app.update(frame);
            }

            BENCHMARK(std::format("{} emitters, {} particles, {}", emitter_count,
                                  ember::particles::particle_count(app.world()),
                                  use_simd ? "simd" : "scalar"))
            {
                app.update(frame);
                return ember::particles::particle_count(app.world());
            };
        }
    }
}
