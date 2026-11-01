export module ember.particles:particle_spawner;

import :particle_pool;
import ember.common;
import ember.core;
import ember.ecs;
import std;

export namespace ember::particles
{

/// Emits the particles of one `ecs::Emitter` into its pool over time.
///
/// Continuous emission usually asks for a fraction of a particle per update, for example
/// 0.35 at 50 particles per second and 144 frames per second. The fraction is kept and added
/// to the next update, so over time the emitter emits exactly `emission_rate` particles per
/// second.
class ParticleSpawner
{
public:
    /// Creates a spawner with no fraction of a particle carried over.
    /// @param seed Start value of the random numbers; the same seed gives the same particles.
    explicit ParticleSpawner(std::uint32_t seed);

    /// Emits the particles due in this update: first `burst_count` particles, then
    /// continuous emission at `emission_rate` while `is_emitting` is true. Every particle
    /// starts at the origin with a random speed between `min_speed` and `max_speed` and a
    /// random direction at most `spread` away from `direction`.
    /// @param emitter Emitter settings. Its `burst_count` is reset to 0.
    /// @param origin Position of the emitter in world units.
    /// @param delta Time since the previous update.
    /// @param pool Pool that receives the particles. Particles that do not fit are dropped.
    /// @return Number of particles added to the pool.
    std::size_t spawn(ecs::Emitter& emitter, Vec2 origin, core::Seconds delta, ParticlePool& pool);

private:
    // Particles owed by continuous emission that were not emitted yet; always less than one
    // after spawn().
    float m_pending = 0.0f;
    // A small generator, because every emitter has its own.
    std::minstd_rand m_random{};
};

} // namespace ember::particles
