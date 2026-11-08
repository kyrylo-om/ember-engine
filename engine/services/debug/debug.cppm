// Development tools: the in-game overlay, the console and the entity inspector.
export module ember.debug;

export import :console;
export import :debug_overlay;
export import :inspector;

import ember.core;
import ember.platform;
import std;

export namespace ember::debug
{

/// Settings of the debug plugin.
struct DebugSettings
{
    /// Key that shows and hides the overlay.
    platform::Key toggle_key = platform::Key::Grave;
    /// Whether the overlay is shown from the start.
    bool is_visible = false;
    /// Whether console commands are also read from the terminal the game runs in.
    bool is_terminal_enabled = true;
    /// Number of log messages kept by the overlay.
    std::size_t max_log_lines = 200;
};

/// Adds the debug overlay, the console and the inspector.
///
/// `build()` inserts the resources `Console` and `DebugOverlay`, starts reading the terminal
/// if enabled, and adds these console commands:
/// - `help`: lists the commands;
/// - `quit`: asks the application to exit;
/// - `entities`: lists the entities with a name;
/// - `inspect <name>`: selects an entity by name and lists its components;
/// - `log_level <trace|debug|info|warning|error>`: sets the lowest level of written logs.
///
/// Its systems:
/// - `Stage::PreUpdate`: show or hide the overlay with `DebugSettings::toggle_key` and run
///   the queued console commands;
/// - `Stage::Render`: draw the overlay while it is visible.
///
/// The plugin must be added after the platform plugin, and after all plugins that draw in
/// `Stage::Render`, so that the overlay is drawn on top.
/// @note Meant for development: a game adds it only to its debug build.
struct DebugPlugin
{
    /// Settings of the overlay and the console.
    DebugSettings settings{};

    /// Inserts the resources, adds the commands and the systems.
    /// @param app Application to add the resources and systems to.
    void build(core::Application& app) const;
};

} // namespace ember::debug
