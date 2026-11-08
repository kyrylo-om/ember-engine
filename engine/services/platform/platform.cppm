// Window, drawing, audio and input: the module that wraps Raylib for the engine. Its interfaces
// use only Ember types, so the rest of the engine does not depend on Raylib.
export module ember.platform;

export import :audio;
export import :graphics;
export import :image;
export import :input;
export import :platform_error;
export import :window;

import ember.common;
import ember.core;
import std;

export namespace ember::platform
{

/// Settings of the platform plugin.
struct PlatformSettings
{
    /// Settings of the window.
    WindowSettings window{};
    /// Color that fills the window at the start of every frame.
    Color clear_color = Color::black;
    /// Whether to open the audio device. Without it there is no `Audio` resource.
    bool is_audio_enabled = true;
};

/// Opens the window and the audio device and adds the systems that run them.
///
/// `build()` inserts the resources `Window`, `Graphics`, `Input` and, if audio is enabled
/// and a device can be opened, `Audio`. It must be the first plugin, because other plugins
/// load textures and sounds while they are built. Its systems:
/// - `Stage::PreUpdate`: read the keyboard and mouse into `Input`, decode playing music,
///   and publish `core::ExitRequested` when the window is asked to close;
/// - `Stage::PreRender`: start the frame with `PlatformSettings::clear_color`;
/// - `Stage::PostRender`: show the frame.
///
/// If the window cannot be opened, the error is logged and the application exits.
struct PlatformPlugin
{
    /// Settings of the window, the frame and audio.
    PlatformSettings settings{};

    /// Opens the window and the audio device, inserts the resources and adds the systems.
    /// @param app Application to add the resources and systems to.
    void build(core::Application& app) const;
};

} // namespace ember::platform
