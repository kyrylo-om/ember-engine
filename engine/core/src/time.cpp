module ember.core;

import std;

namespace ember::core
{

FixedTimestep::FixedTimestep(Seconds step, int max_steps) : m_step{step}, m_max_steps{max_steps}
{
}

int FixedTimestep::advance(Seconds frame_time)
{
    // Clamping the accumulator instead of the result also drops the leftover of a slow
    // frame, so the next frame does not start with a backlog.
    const Seconds max_accumulated = m_step * static_cast<float>(m_max_steps);
    m_accumulator = std::min(m_accumulator + frame_time, max_accumulated);

    const int steps = static_cast<int>(m_accumulator / m_step);
    m_accumulator -= m_step * static_cast<float>(steps);
    return steps;
}

float FixedTimestep::alpha() const
{
    return m_accumulator / m_step;
}

Seconds FixedTimestep::step() const
{
    return m_step;
}

int FixedTimestep::max_steps() const
{
    return m_max_steps;
}

} // namespace ember::core
