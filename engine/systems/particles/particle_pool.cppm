export module ember.particles:particle_pool;

import ember.common;
import ember.core;
import std;

export namespace ember::particles
{

/// State of one live particle, as returned by `ParticlePool::particle()`.
struct Particle
{
    /// Position in world units.
    Vec2 position{};
    /// Velocity in world units per second.
    Vec2 velocity{};
    /// Time since the particle was emitted.
    core::Seconds age{};
};

/// Stores the live particles of one emitter and moves them.
///
/// Every field is kept in its own array (a structure of arrays), so the simulation reads and
/// writes contiguous floats and can process several particles with one `std::simd`
/// instruction. The order of the particles is not kept: removing one moves the last particle
/// into its place.
/// @note The capacity is fixed when the pool is created, so the pool never allocates while
/// the game runs.
class ParticlePool
{
public:
    /// Creates an empty pool.
    /// @param capacity Maximum number of live particles.
    explicit ParticlePool(std::size_t capacity);

    /// Adds a particle with age 0.
    /// @param position Position in world units.
    /// @param velocity Velocity in world units per second.
    /// @return False if the pool is full; the particle is not added then.
    bool emit(Vec2 position, Vec2 velocity);

    /// Moves and ages all particles with `std::simd`, then removes those whose age reached
    /// the lifetime.
    /// @param delta Time to simulate.
    /// @param acceleration Acceleration of every particle in world units per second squared.
    /// @param lifetime Age at which a particle is removed.
    void simulate_simd(core::Seconds delta, Vec2 acceleration, core::Seconds lifetime);

    /// Does the same as `simulate_simd()` one particle at a time. Used to test the SIMD
    /// version and to compare their performance.
    /// @param delta Time to simulate.
    /// @param acceleration Acceleration of every particle in world units per second squared.
    /// @param lifetime Age at which a particle is removed.
    void simulate_scalar(core::Seconds delta, Vec2 acceleration, core::Seconds lifetime);

    /// Removes all particles.
    void clear();

    /// Returns a live particle.
    /// @param index Index of the particle, less than `size()`.
    /// @return Copy of the particle's state.
    Particle particle(std::size_t index) const;

    /// Returns the number of live particles.
    std::size_t size() const;

    /// Returns the maximum number of live particles.
    std::size_t capacity() const;

private:
    // Moves and ages the particles from index first to the end, one at a time.
    void move_scalar(std::size_t first, float dt, Vec2 velocity_change);
    void remove_expired(core::Seconds lifetime);

    std::size_t m_size = 0;
    // All arrays have the length of the capacity; only the first m_size elements are live.
    std::vector<float> m_position_x{};
    std::vector<float> m_position_y{};
    std::vector<float> m_velocity_x{};
    std::vector<float> m_velocity_y{};
    std::vector<float> m_age{};
};

} // namespace ember::particles
