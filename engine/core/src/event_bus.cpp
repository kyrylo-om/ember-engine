module ember.core;

import std;

namespace ember::core
{

EventBus::EventBus() = default;

EventBus::~EventBus() = default;

// A mutex cannot be moved, so only the channels move and the new bus gets its own mutex.
// Moving is not synchronized: no system may use either bus at that time.
EventBus::EventBus(EventBus&& other) noexcept : m_channels{std::move(other.m_channels)}
{
}

EventBus& EventBus::operator=(EventBus&& other) noexcept
{
    m_channels = std::move(other.m_channels);
    return *this;
}

void EventBus::advance_frame()
{
    const std::scoped_lock lock{m_channels_mutex};
    for (const auto& channel : m_channels | std::views::values)
    {
        channel->advance_frame();
    }
}

} // namespace ember::core
