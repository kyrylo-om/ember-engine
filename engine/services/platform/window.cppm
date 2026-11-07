export module ember.platform:window;

import :platform_error;
import ember.common;
import std;

export namespace ember::platform
{

/// Settings of the window, used when it is opened.
struct WindowSettings
{
    /// Text in the title bar.
    std::string title = "Ember";
    /// Width of the drawing area in pixels.
    int width = 1280;
    /// Height of the drawing area in pixels.
    int height = 720;
    /// Whether the user can resize the window.
    bool is_resizable = false;
    /// Whether the window starts in fullscreen mode.
    bool is_fullscreen = false;
    /// Whether showing a frame waits for the vertical refresh of the monitor, which limits
    /// the frame rate to the refresh rate and prevents tearing.
    bool is_vsync = true;
};

/// The window of the game and its graphics context.
///
/// Only one window can be open at a time. The window is a world resource; `Graphics` needs
/// it to be open, so it is inserted into the world before `Graphics` and destroyed after it.
/// @note Must be used only from the main thread.
class Window
{
public:
    /// Opens the window.
    /// @param settings Size, title and mode of the window.
    /// @return The open window, or `PlatformError::WindowUnavailable` if it cannot be
    /// created or another window is already open.
    [[nodiscard]] static std::expected<Window, PlatformError> open(const WindowSettings& settings);

    /// Closes the window.
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /// Takes over the open window; the moved-from object no longer owns it.
    Window(Window&& other) noexcept;

    /// Closes this window, then takes over the window of `other`.
    Window& operator=(Window&& other) noexcept;

    /// Returns true after the user asked to close the window, for example with its close
    /// button.
    bool should_close() const;

    /// Returns the size of the drawing area in pixels.
    Vec2 size() const;

    /// Sets the text in the title bar.
    void set_title(std::string_view title);

    /// Returns true if the window covers the whole monitor.
    bool is_fullscreen() const;

    /// Switches between fullscreen and windowed mode.
    void set_fullscreen(bool is_fullscreen);

private:
    // Created only by open(), so an existing Window always owns an open window unless it
    // was moved from.
    Window() = default;

    bool m_is_open = false;
};

} // namespace ember::platform
