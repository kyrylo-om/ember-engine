export module ember.core:scheduler;

import ember.ecs;
import std;

export namespace ember::core
{

/// Parts of a frame in which systems run, listed in the order they run.
///
/// `Startup` and `Shutdown` run once, all other stages run every frame.
enum class Stage
{
    /// Once, before the first frame: creating the window, loading assets and scenes.
    Startup,
    /// Start of the frame: reading input.
    PreUpdate,
    /// Zero or several times per frame with a fixed time step: physics.
    FixedUpdate,
    /// Once per frame: game logic, particle simulation.
    Update,
    /// After the game logic: camera movement, cleanup.
    PostUpdate,
    /// Preparing to draw: starting the frame image.
    PreRender,
    /// Drawing the world: sprites, particles, debug overlay.
    Render,
    /// Finishing the frame: showing the image on the screen.
    PostRender,
    /// Once, after the last frame: releasing resources.
    Shutdown
};

/// A system: a function that runs over the world once every time its stage runs.
using System = std::move_only_function<void(ecs::World& world)>;

/// Stores the systems of every stage and runs them.
///
/// Systems of a stage run in the order they were added. Deferred commands recorded by the
/// systems are applied to the world after each stage, so the next stage sees their result.
class Scheduler
{
public:
    /// Creates a scheduler without systems.
    Scheduler();
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    /// Adds a system to the end of a stage.
    /// @param stage Stage in which the system runs.
    /// @param name Name of the system, used in logs and profiling.
    /// @param system The system to run.
    void add_system(Stage stage, std::string_view name, System system);

    /// Runs all systems of a stage, then applies the deferred commands.
    /// @param stage Stage to run.
    /// @param world World passed to the systems.
    void run_stage(Stage stage, ecs::World& world);

    /// Returns the number of systems in a stage.
    /// @param stage Stage whose systems are counted.
    /// @return Number of systems added to the stage.
    std::size_t system_count(Stage stage) const;
};

} // namespace ember::core
