export module ember.core:event_bus;

import std;

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
    /// @note The span is valid until the next call to `advance_frame()`.
    template <typename E>
    [[nodiscard]] std::span<const E> read() const;

    /// Makes the events published during this frame readable and drops the events of the
    /// previous frame. The application calls it at the start of every frame.
    void advance_frame();
};

} // namespace ember::core
