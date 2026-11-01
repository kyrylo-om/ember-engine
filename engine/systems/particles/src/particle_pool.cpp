module;

// The std module of GCC 16 does not export std::simd yet, so its header is included here,
// where it stays private to this file.
#include <simd>

module ember.particles;

import ember.common;
import ember.core;
import std;

namespace ember::particles
{

namespace
{

using FloatVector = std::simd::vec<float>;
constexpr auto simd_width = static_cast<std::size_t>(FloatVector::size());

} // namespace

// Parentheses on purpose: braces would create vectors that hold the single value capacity.
ParticlePool::ParticlePool(std::size_t capacity)
    : m_position_x(capacity), m_position_y(capacity), m_velocity_x(capacity),
      m_velocity_y(capacity), m_age(capacity)
{
}

bool ParticlePool::emit(Vec2 position, Vec2 velocity)
{
    if (m_size == capacity())
    {
        return false;
    }

    m_position_x[m_size] = position.x;
    m_position_y[m_size] = position.y;
    m_velocity_x[m_size] = velocity.x;
    m_velocity_y[m_size] = velocity.y;
    m_age[m_size] = 0.0f;
    ++m_size;
    return true;
}

void ParticlePool::simulate_simd(core::Seconds delta, Vec2 acceleration, core::Seconds lifetime)
{
    const float dt = delta.count();
    // The same operations in the same order as in move_scalar(), so both versions give the
    // same results.
    const Vec2 velocity_change = acceleration * dt;

    // Only full vectors: full-width loads and stores are the fastest, and partial_store of
    // GCC 16 writes chunks of one or two elements incorrectly. The particles left over,
    // fewer than one vector, are moved by the scalar code.
    const std::size_t vector_end = m_size - m_size % simd_width;
    for (std::size_t first = 0; first < vector_end; first += simd_width)
    {
        const auto chunk = [first](std::vector<float>& values)
        {
            return std::span{values}.subspan(first, simd_width);
        };

        FloatVector velocity_x = std::simd::unchecked_load<FloatVector>(chunk(m_velocity_x));
        FloatVector velocity_y = std::simd::unchecked_load<FloatVector>(chunk(m_velocity_y));
        FloatVector position_x = std::simd::unchecked_load<FloatVector>(chunk(m_position_x));
        FloatVector position_y = std::simd::unchecked_load<FloatVector>(chunk(m_position_y));
        FloatVector age = std::simd::unchecked_load<FloatVector>(chunk(m_age));

        velocity_x += velocity_change.x;
        velocity_y += velocity_change.y;
        position_x += velocity_x * dt;
        position_y += velocity_y * dt;
        age += dt;

        std::simd::unchecked_store(velocity_x, chunk(m_velocity_x));
        std::simd::unchecked_store(velocity_y, chunk(m_velocity_y));
        std::simd::unchecked_store(position_x, chunk(m_position_x));
        std::simd::unchecked_store(position_y, chunk(m_position_y));
        std::simd::unchecked_store(age, chunk(m_age));
    }
    move_scalar(vector_end, dt, velocity_change);

    remove_expired(lifetime);
}

void ParticlePool::simulate_scalar(core::Seconds delta, Vec2 acceleration, core::Seconds lifetime)
{
    const float dt = delta.count();
    move_scalar(0, dt, acceleration * dt);
    remove_expired(lifetime);
}

void ParticlePool::move_scalar(std::size_t first, float dt, Vec2 velocity_change)
{
    for (std::size_t index = first; index < m_size; ++index)
    {
        m_velocity_x[index] += velocity_change.x;
        m_velocity_y[index] += velocity_change.y;
        m_position_x[index] += m_velocity_x[index] * dt;
        m_position_y[index] += m_velocity_y[index] * dt;
        m_age[index] += dt;
    }
}

void ParticlePool::clear()
{
    m_size = 0;
}

Particle ParticlePool::particle(std::size_t index) const
{
    return {
        .position = {m_position_x[index], m_position_y[index]},
        .velocity = {m_velocity_x[index], m_velocity_y[index]},
        .age = core::Seconds{m_age[index]},
    };
}

std::size_t ParticlePool::size() const
{
    return m_size;
}

std::size_t ParticlePool::capacity() const
{
    return m_age.size();
}

void ParticlePool::remove_expired(core::Seconds lifetime)
{
    const float max_age = lifetime.count();
    std::size_t index = 0;
    while (index < m_size)
    {
        if (m_age[index] < max_age)
        {
            ++index;
            continue;
        }

        // The order of the particles does not matter, so the last one fills the gap instead
        // of all later ones moving down. The moved particle is checked in the next iteration.
        --m_size;
        for (std::vector<float>* values :
             {&m_position_x, &m_position_y, &m_velocity_x, &m_velocity_y, &m_age})
        {
            (*values)[index] = (*values)[m_size];
        }
    }
}

} // namespace ember::particles
