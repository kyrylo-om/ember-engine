// Drawing of the world: sprites and shapes through the active camera, skipped when outside
// the view and sorted by layer.
export module ember.render;

export import :active_camera;
export import :culling;
export import :shape_queue;

import ember.core;
import std;

export namespace ember::render
{

/// Common values of `ecs::Sprite::layer` and `Shape::layer`. Any other value can be used
/// too; these give the usual order names.
namespace layer
{

/// Behind everything else, for example the sky.
inline constexpr int background = -100;
/// Default layer of the game world.
inline constexpr int world = 0;
/// In front of the game world, for example foliage that hides the player.
inline constexpr int foreground = 100;

} // namespace layer

/// Settings of the render system, stored as a world resource.
struct RenderSettings
{
    /// Whether sprites outside the view are skipped. Turning it off helps to check that
    /// culling hides nothing visible.
    bool is_culling_enabled = true;
};

/// Draws the world in `Stage::Render`.
///
/// `build()` inserts the resources `RenderSettings`, `ActiveCamera` and `ShapeQueue` and
/// adds one system to `Stage::Render`. Every frame it draws, through the view of the
/// active camera, every entity with an `ecs::Transform` and an `ecs::Sprite` whose bounds
/// overlap the visible area, and every shape in the `ShapeQueue`. They are drawn from the
/// lowest layer to the highest; within a layer sprites come before shapes, and shapes keep
/// the order they were added in. Then the shape queue is cleared.
///
/// The plugin must be added after the platform plugin, which provides `platform::Window`
/// and `platform::Graphics`. Systems that other plugins add to `Stage::Render` later, such
/// as particles or the debug overlay, draw on top of all layers.
struct RenderPlugin
{
    /// Settings inserted into the world as a resource.
    RenderSettings settings{};

    /// Inserts the resources and adds the render system.
    /// @param app Application to add the resources and the system to.
    void build(core::Application& app) const;
};

} // namespace ember::render
