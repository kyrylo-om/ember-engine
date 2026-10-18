export module ember.core:application;

import :scheduler;
import :time;
import ember.ecs;
import std;

export namespace ember::core
{

class Application;

/// A plugin adds a group of systems and resources to an application, usually one module.
///
/// Any type with a member function `void build(Application& app) const` is a plugin.
/// Plugins usually are structs whose fields are the settings of the module.
template <typename P>
concept Plugin = requires(const P& plugin, Application& app) { plugin.build(app); };

/// Settings of the main loop.
struct ApplicationConfig
{
    /// Duration of one step of `Stage::FixedUpdate`.
    Seconds fixed_step{1.0f / 60.0f};
    /// Longest frame time passed to the systems. Longer frames, for example after a
    /// breakpoint, are treated as this long.
    Seconds max_frame_time{0.25f};
    /// Maximum number of fixed steps per frame.
    int max_fixed_steps = 5;
};

/// Event that asks the application to stop after the current frame, for example when the
/// window is closed.
struct ExitRequested
{
};

/// Owns the world and the scheduler and runs the main loop.
///
/// A game creates an application, adds plugins and its own systems, then calls `run()`.
/// The world contains a `Time` and an `EventBus` resource from the start.
class Application
{
public:
    /// Creates an application with an empty world and no systems.
    /// @param config Settings of the main loop.
    explicit Application(ApplicationConfig config = {});
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /// Adds a plugin by calling its `build()` function immediately.
    /// @tparam P Type of the plugin.
    /// @param plugin The plugin to add.
    /// @return This application, so calls can be chained.
    template <Plugin P>
    Application& add_plugin(const P& plugin);

    /// Adds a system to the end of a stage.
    /// @param stage Stage in which the system runs.
    /// @param name Name of the system, used in logs and profiling.
    /// @param system The system to run.
    /// @return This application, so calls can be chained.
    Application& add_system(Stage stage, std::string_view name, System system);

    /// Returns the world with all entities and resources.
    ecs::World& world();

    /// Returns the world with all entities and resources.
    const ecs::World& world() const;

    /// Returns the scheduler with all systems.
    Scheduler& scheduler();

    /// Runs `Stage::Startup`, then frames until an exit is requested, then
    /// `Stage::Shutdown`. Frame times are measured with `std::chrono::steady_clock`.
    void run();

    /// Runs one frame as if the given time had passed since the previous frame. Runs
    /// `Stage::Startup` first if it has not run yet. Used by tests and benchmarks.
    /// @param frame_time Time passed to the systems as the length of the frame.
    void update(Seconds frame_time);

    /// Asks the application to stop after the current frame.
    void request_exit();

    /// Returns true until an exit is requested.
    bool is_running() const;
};

} // namespace ember::core
