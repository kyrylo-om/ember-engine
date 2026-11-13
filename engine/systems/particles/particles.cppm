// Particle effects for the Emitter components of ecs: settings and the plugin that simulates and
// draws them.
export module ember.particles;

export import :particle_pool;
export import :particle_spawner;

import ember.core;
import ember.ecs;
import std;

export namespace ember::particles
{

/// Settings of the particle system, stored as a world resource.
struct ParticleSettings
{
    /// Maximum number of live particles per emitter. While the pool is full, the emitter
    /// emits no new particles. Read when the pool of an emitter is created.
    std::size_t max_particles_per_emitter = 1024;
    /// Whether particles are simulated with `std::simd`. The scalar version is used for
    /// testing and for comparing performance.
    bool use_simd = true;
};

/// Simulates the particles of all emitters in `Stage::Update`.
///
/// Every entity with an `ecs::Transform` and an `ecs::Emitter` gets its own particle pool
/// when the system first sees it. Each update moves and ages the live particles, removes
/// those older than `ecs::Emitter::lifetime`, then emits new ones at the position of the
/// entity. When the entity is destroyed or loses one of the two components, its particles
/// disappear.
struct ParticlePlugin
{
    /// Settings inserted into the world as a resource.
    ParticleSettings settings{};

    /// Inserts the settings and adds the particle system.
    /// @param app Application to add the system to.
    void build(core::Application& app) const;
};

/// Returns the number of live particles of all emitters.
/// @param world World with the emitters.
/// @return Number of live particles; 0 if the world has no particle plugin.
std::size_t particle_count(const ecs::World& world);

} // namespace ember::particles
