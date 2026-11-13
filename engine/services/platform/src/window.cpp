module ember.platform;

import :raylib_support;
import ember.common;
import raylib;
import std;

namespace ember::platform
{

namespace
{

// Raylib closes the window on Escape unless the exit key is set to "no key". The game
// decides what Escape does.
constexpr int no_exit_key = 0;

unsigned int config_flags(const WindowSettings& settings)
{
    unsigned int flags = 0;
    if (settings.is_resizable)
    {
        flags |= raylib::ConfigFlags::FLAG_WINDOW_RESIZABLE;
    }
    if (settings.is_fullscreen)
    {
        flags |= raylib::ConfigFlags::FLAG_FULLSCREEN_MODE;
    }
    if (settings.is_vsync)
    {
        flags |= raylib::ConfigFlags::FLAG_VSYNC_HINT;
    }
    return flags;
}

} // namespace

std::expected<Window, PlatformError> Window::open(const WindowSettings& settings)
{
    if (raylib::Window::IsReady())
    {
        return std::unexpected{PlatformError::WindowUnavailable};
    }

    try
    {
        // Only warnings and errors of Raylib are printed; its startup report is not.
        raylib::Window::Init(settings.width, settings.height, settings.title,
                             config_flags(settings), raylib::TraceLogLevel::LOG_WARNING);
    }
    catch (const raylib::RaylibException&)
    {
        return std::unexpected{PlatformError::WindowUnavailable};
    }
    raylib::Window::SetExitKey(no_exit_key);

    Window window;
    window.m_is_open = true;
    return window;
}

Window::~Window()
{
    if (m_is_open)
    {
        raylib::Window::Close();
    }
}

Window::Window(Window&& other) noexcept : m_is_open{std::exchange(other.m_is_open, false)}
{
}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other)
    {
        if (m_is_open)
        {
            raylib::Window::Close();
        }
        m_is_open = std::exchange(other.m_is_open, false);
    }
    return *this;
}

bool Window::should_close() const
{
    return raylib::Window::ShouldClose();
}

Vec2 Window::size() const
{
    return to_vec2(raylib::Window::GetSize());
}

void Window::set_title(std::string_view title)
{
    raylib_window().SetTitle(std::string{title});
}

bool Window::is_fullscreen() const
{
    return raylib::Window::IsFullscreen();
}

void Window::set_fullscreen(bool is_fullscreen)
{
    raylib_window().SetFullscreen(is_fullscreen);
}

} // namespace ember::platform
