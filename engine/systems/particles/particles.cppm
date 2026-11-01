// Particle effects for the Emitter components of ecs: settings and the plugin that simulates and
// draws them.
export module ember.particles;

import ember.core;
import ember.ecs;
import std;

export namespace ember::particles
{

/// Settings of the particle system, stored as a world resource.
struct ParticleSettings
{
    /// Maximum number of live particles per emitter. While the pool is full, the emitter
    /// emits no new particles.
    std::size_t max_particles_per_emitter = 1024;
    /// Whether particles are simulated with `std::simd`. The scalar version is used for
    /// testing and for comparing performance.
    bool use_simd = true;
};

/// Adds particle simulation to `Stage::Update` and particle drawing to `Stage::Render`.
struct ParticlePlugin
{
    /// Settings inserted into the world as a resource.
    ParticleSettings settings{};

    /// Inserts the settings and adds the particle systems.
    /// @param app Application to add the systems to.
    void build(core::Application& app) const;
};

/// Returns the number of live particles of all emitters.
/// @param world World with the emitters.
/// @return Number of live particles.
std::size_t particle_count(const ecs::World& world);

} // namespace ember::particles
