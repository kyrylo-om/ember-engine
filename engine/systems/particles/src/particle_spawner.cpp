module ember.particles;

import ember.common;
import ember.core;
import ember.ecs;
import std;

namespace ember::particles
{

ParticleSpawner::ParticleSpawner(std::uint32_t seed) : m_random{seed}
{
}

std::size_t ParticleSpawner::spawn(ecs::Emitter& emitter, Vec2 origin, core::Seconds delta,
                                   ParticlePool& pool)
{
    std::size_t due = emitter.burst_count;
    emitter.burst_count = 0;

    if (emitter.is_emitting)
    {
        // A negative rate emits nothing instead of making the count negative.
        m_pending += std::max(emitter.emission_rate, 0.0f) * delta.count();
        const float whole = std::floor(m_pending);
        m_pending -= whole;
        due += static_cast<std::size_t>(whole);
    }
    else
    {
        // A fraction left from before must not add a particle when emission starts again.
        m_pending = 0.0f;
    }

    // Emitters are loaded from scene files, so swapped or negative ranges are corrected here
    // instead of breaking the preconditions of the distributions.
    const auto [min_speed, max_speed] = std::minmax(emitter.min_speed, emitter.max_speed);
    const float spread = std::abs(emitter.spread);
    std::uniform_real_distribution<float> speed_distribution{min_speed, max_speed};
    std::uniform_real_distribution<float> angle_distribution{emitter.direction - spread,
                                                             emitter.direction + spread};

    std::size_t emitted = 0;
    while (emitted < due)
    {
        const float angle = angle_distribution(m_random);
        const float speed = speed_distribution(m_random);
        // y points down, so a growing angle turns clockwise on the screen.
        if (!pool.emit(origin, Vec2{std::cos(angle), std::sin(angle)} * speed))
        {
            break;
        }
        ++emitted;
    }
    return emitted;
}

} // namespace ember::particles
