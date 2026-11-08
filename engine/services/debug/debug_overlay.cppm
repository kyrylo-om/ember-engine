export module ember.debug:debug_overlay;

import ember.core;
import ember.ecs;
import std;

export namespace ember::debug
{

/// Log message kept by the overlay.
struct LogLine
{
    /// Level the message was written with.
    core::LogLevel level = core::LogLevel::Info;
    /// Text of the message.
    std::string message{};
};

/// State of the in-game debug overlay: whether it is shown, the statistics, the last log
/// messages and the entity selected in the inspector.
///
/// The overlay shows the frame time, the statistics, the log with a console input line and
/// the inspector. The debug plugin draws it in `Stage::Render` while it is visible.
class DebugOverlay
{
public:
    /// Creates a hidden overlay and starts keeping log messages.
    /// @param max_log_lines Number of log messages kept; older ones are dropped.
    explicit DebugOverlay(std::size_t max_log_lines);

    /// Stops keeping log messages.
    ~DebugOverlay();

    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    /// Takes over the state; log messages keep arriving.
    DebugOverlay(DebugOverlay&& other) noexcept;

    /// Stops keeping log messages for this overlay, then takes over the state of `other`.
    DebugOverlay& operator=(DebugOverlay&& other) noexcept;

    /// Returns true while the overlay is shown.
    bool is_visible() const;

    /// Shows or hides the overlay.
    void set_visible(bool is_visible);

    /// Sets a line of the statistics panel, for example the number of particles. Values are
    /// kept until they are changed or removed, so they are usually set every frame.
    /// @param name Label of the line; a line with the same label is replaced.
    /// @param value Text shown next to the label.
    void set_stat(std::string_view name, std::string value);

    /// Removes a line of the statistics panel.
    /// @param name Label of the line. Nothing happens if there is no such line.
    void remove_stat(std::string_view name);

    /// Returns the kept log messages, the oldest first.
    /// @return Copy of the messages, because new ones can arrive from any thread.
    std::vector<LogLine> log_lines() const;

    /// Returns the entity shown in the inspector; the null entity if none is selected.
    ecs::Entity selected_entity() const;

    /// Selects the entity shown in the inspector.
    /// @param entity Entity to show; the null entity clears the selection.
    void select(ecs::Entity entity);

private:
    // The log sink refers to this state, so it needs a stable address while the overlay
    // itself is moved into the world.
    struct State;
    std::unique_ptr<State> m_state;
};

} // namespace ember::debug
