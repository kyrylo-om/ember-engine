export module ember.render:active_camera;

import ember.common;
import ember.ecs;
import ember.platform;
import std;

export namespace ember::render
{

/// Selects the camera the world is drawn with; a world resource inserted by the render
/// plugin.
///
/// If `entity` is not alive or has no `ecs::Camera`, another entity with an `ecs::Camera`
/// is used; which one is unspecified, so a world with several cameras sets `entity`.
/// Without any camera, a default `ecs::Camera` is used, which shows the world origin in
/// the center of the window.
struct ActiveCamera
{
    /// Entity whose `ecs::Camera` is used.
    ecs::Entity entity{};
};

/// Returns the camera the world is drawn with, chosen as described for `ActiveCamera`.
/// @param world World with the cameras and, optionally, an `ActiveCamera` resource.
/// @return Copy of the chosen camera.
ecs::Camera active_camera(const ecs::World& world);

/// Returns the view that shows a camera in the center of the window.
/// @param camera Camera to show.
/// @param screen_size Size of the window in pixels.
/// @return View for `platform::Graphics::begin_view()` and the conversions between the
/// screen and the world, such as `platform::screen_to_world()`.
platform::View make_view(const ecs::Camera& camera, Vec2 screen_size);

/// Returns the view of the active camera for the current size of the window, for example to
/// find the point of the world under the mouse.
/// @param world World with the cameras and a `platform::Window` resource.
/// @return View of the camera returned by `active_camera()`.
platform::View active_view(const ecs::World& world);

} // namespace ember::render
