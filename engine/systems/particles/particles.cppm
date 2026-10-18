// Particle effects: emitter components, settings and the plugin that simulates and draws them.
export module ember.particles;

import ember.common;
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

/// Component that emits particles from the position of its entity.
///
/// Particles move in world space: moving the emitter does not move the particles already
/// emitted. Color and size change linearly from the start to the end value over the life
/// of a particle.
struct ParticleEmitter
{
    /// Number of particles emitted per second while `is_emitting` is true.
    float emission_rate = 50.0f;
    /// Number of particles emitted at once in the next update; reset to 0 after that.
    std::uint32_t burst_count = 0;
    /// Whether particles are emitted continuously at `emission_rate`.
    bool is_emitting = true;
    /// How long every particle lives.
    core::Seconds lifetime{1.0f};
    /// Smallest initial speed of a particle, in pixels per second.
    float min_speed = 50.0f;
    /// Largest initial speed of a particle, in pixels per second.
    float max_speed = 100.0f;
    /// Middle direction of emission, as an angle in radians from the positive x axis.
    float direction = 0.0f;
    /// Largest angle in radians between the direction of a particle and `direction`.
    float spread = std::numbers::pi_v<float>;
    /// Acceleration of every particle in pixels per second squared, for example gravity.
    Vec2 acceleration{};
    /// Color of a particle when it is emitted.
    Color start_color{};
    /// Color of a particle at the end of its life.
    Color end_color{};
    /// Size of a particle in pixels when it is emitted.
    float start_size = 4.0f;
    /// Size of a particle in pixels at the end of its life.
    float end_size = 0.0f;
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
