export module ember.core:time;

import std;

export namespace ember::core
{

/// Duration in seconds stored as `float`, the unit of all frame times in the engine.
using Seconds = std::chrono::duration<float>;

/// Frame timing, available to systems as a world resource.
///
/// The application updates it at the start of every frame. Systems in `Stage::FixedUpdate`
/// use `fixed_delta`, all other systems use `delta`.
struct Time
{
    /// Time since the start of the previous frame, limited by
    /// `ApplicationConfig::max_frame_time`.
    Seconds delta{};
    /// Duration of one fixed step.
    Seconds fixed_delta{};
    /// Time since the first frame. Stored as `double`, so it stays precise in long sessions.
    std::chrono::duration<double> elapsed{};
    /// Part of a fixed step that has passed but is not simulated yet, from 0 to 1.
    /// Rendering uses it to interpolate between the last two fixed steps.
    float fixed_alpha = 0.0f;
    /// Number of the current frame, starting from 0.
    std::uint64_t frame = 0;
};

/// Splits variable frame times into a whole number of fixed steps.
///
/// Frame time that is not enough for a full step is kept and added to the next frame, so
/// the simulation advances by the same total time as the real clock.
class FixedTimestep
{
public:
    /// Creates a timestep with no accumulated time.
    /// @param step Duration of one fixed step; must be greater than zero.
    /// @param max_steps Maximum number of steps per frame. Time above this limit is dropped,
    ///                  so a slow frame does not cause even slower frames after it.
    FixedTimestep(Seconds step, int max_steps);

    /// Adds the time of a frame and returns how many fixed steps to run in this frame.
    /// @param frame_time Time that passed since the previous call.
    /// @return Number of steps, from 0 to `max_steps()`.
    [[nodiscard]] int advance(Seconds frame_time);

    /// Returns the accumulated time that is not enough for a full step, as a fraction of a
    /// step from 0 to 1.
    float alpha() const;

    /// Returns the duration of one fixed step.
    Seconds step() const;

    /// Returns the maximum number of steps per frame.
    int max_steps() const;
};

} // namespace ember::core
