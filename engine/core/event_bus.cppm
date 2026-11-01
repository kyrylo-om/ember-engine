export module ember.core:event_bus;

import std;

namespace ember::core
{

// Events of one type behind a base without that type, so the bus keeps all channels in one map.
class EventChannelBase
{
public:
    EventChannelBase() = default;
    virtual ~EventChannelBase() = default;

    EventChannelBase(const EventChannelBase&) = delete;
    EventChannelBase& operator=(const EventChannelBase&) = delete;
    EventChannelBase(EventChannelBase&&) = delete;
    EventChannelBase& operator=(EventChannelBase&&) = delete;

    virtual void advance_frame() = 0;
};

template <typename E>
class EventChannel final : public EventChannelBase
{
public:
    void publish(E event)
    {
        const std::scoped_lock lock{m_mutex};
        m_pending.push_back(std::move(event));
    }

    // Not locked: only advance_frame() changes the readable events, and it does not run
    // together with the systems.
    std::span<const E> readable() const
    {
        return m_readable;
    }

    void advance_frame() override
    {
        const std::scoped_lock lock{m_mutex};
        // Swapping keeps the capacity of both buffers, so a steady stream of events stops
        // allocating after the first frames.
        std::swap(m_readable, m_pending);
        m_pending.clear();
    }

private:
    std::mutex m_mutex{};
    std::vector<E> m_pending{};
    std::vector<E> m_readable{};
};

} // namespace ember::core

export namespace ember::core
{

/// Delivers events from the systems that publish them to the systems that read them.
///
/// The bus is a world resource. Events are buffered per frame: an event published during
/// a frame can be read during the whole next frame, by every system, in the order it was
/// published. After that frame it is dropped. Any copyable type can be an event; event
/// types shared by several modules are defined in module `ember.common`.
///
/// @note Systems in `Stage::FixedUpdate` run zero or several times per frame, so they can
///       see the events of a frame zero or several times.
class EventBus
{
public:
    /// Creates a bus without events.
    EventBus();
    ~EventBus();

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) noexcept;
    EventBus& operator=(EventBus&&) noexcept;

    /// Publishes an event. It can be read starting from the next frame.
    /// @tparam E Type of the event.
    /// @param event The event to publish.
    /// @note Can be called by systems running in parallel.
    template <typename E>
    void publish(E event);

    /// Returns the events of a type that were published during the previous frame.
    /// @tparam E Type of the events.
    /// @return Events in publishing order; empty if there are none.
    /// @note The span is valid until the next call to `advance_frame()`. Can be called by
    ///       systems running in parallel, also while they publish events.
    template <typename E>
    [[nodiscard]] std::span<const E> read() const;

    /// Makes the events published during this frame readable and drops the events of the
    /// previous frame. The application calls it at the start of every frame.
    /// @note Must not be called while systems publish or read events.
    void advance_frame();

private:
    template <typename E>
    EventChannel<E>& channel();

    template <typename E>
    const EventChannel<E>* find_channel() const;

    // Guards only the map. Each channel guards its own events, so systems that publish
    // different event types do not wait for each other.
    mutable std::shared_mutex m_channels_mutex{};
    std::unordered_map<std::type_index, std::unique_ptr<EventChannelBase>> m_channels{};
};

} // namespace ember::core

namespace ember::core
{

template <typename E>
void EventBus::publish(E event)
{
    // Without const, so publish<const E> and read<const E> use the same channel as E.
    channel<std::remove_cv_t<E>>().publish(std::move(event));
}

template <typename E>
std::span<const E> EventBus::read() const
{
    const auto* found = find_channel<std::remove_cv_t<E>>();
    if (found == nullptr)
    {
        return {};
    }
    return found->readable();
}

template <typename E>
EventChannel<E>& EventBus::channel()
{
    const std::type_index type{typeid(E)};
    {
        // After the first event of a type, publishing takes only this shared lock.
        const std::shared_lock lock{m_channels_mutex};
        if (const auto found = m_channels.find(type); found != m_channels.end())
        {
            return static_cast<EventChannel<E>&>(*found->second);
        }
    }

    const std::scoped_lock lock{m_channels_mutex};
    // Another thread may have created the channel between the two locks.
    const auto [position, is_inserted] = m_channels.try_emplace(type);
    if (is_inserted)
    {
        position->second = std::make_unique<EventChannel<E>>();
    }
    // The channel is owned by a unique_ptr, so the reference stays valid when the map
    // rehashes.
    return static_cast<EventChannel<E>&>(*position->second);
}

template <typename E>
const EventChannel<E>* EventBus::find_channel() const
{
    const std::shared_lock lock{m_channels_mutex};
    const auto found = m_channels.find(std::type_index{typeid(E)});
    if (found == m_channels.end())
    {
        return nullptr;
    }
    return static_cast<const EventChannel<E>*>(found->second.get());
}

} // namespace ember::core
