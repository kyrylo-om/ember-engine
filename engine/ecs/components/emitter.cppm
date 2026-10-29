export module ember.ecs:emitter;

import ember.common;
import std;

export namespace ember::ecs
{

/// Emits particles from `Transform::position` of its entity.
///
/// Particles move in world space: moving the emitter does not move the particles already
/// emitted. Color and size change linearly from the start to the end value over the life
/// of a particle.
struct Emitter
{
    /// Number of particles emitted per second while `is_emitting` is true.
    float emission_rate = 50.0f;
    /// Number of particles emitted at once in the next update; reset to 0 after that.
    std::uint32_t burst_count = 0;
    /// Whether particles are emitted continuously at `emission_rate`.
    bool is_emitting = true;
    /// How long every particle lives.
    std::chrono::duration<float> lifetime{1.0f};
    /// Smallest initial speed of a particle, in world units per second.
    float min_speed = 50.0f;
    /// Largest initial speed of a particle, in world units per second.
    float max_speed = 100.0f;
    /// Middle direction of emission, in radians clockwise from the positive x axis.
    float direction = 0.0f;
    /// Largest angle in radians between the direction of a particle and `direction`.
    float spread = std::numbers::pi_v<float>;
    /// Acceleration of every particle in world units per second squared, for example
    /// gravity.
    Vec2 acceleration{};
    /// Color of a particle when it is emitted.
    Color start_color{};
    /// Color of a particle at the end of its life.
    Color end_color{};
    /// Size of a particle in world units when it is emitted.
    float start_size = 4.0f;
    /// Size of a particle in world units at the end of its life.
    float end_size = 0.0f;
};

} // namespace ember::ecs
