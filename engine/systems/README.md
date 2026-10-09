# Systems

Modules that add behavior running every frame over ECS components: moving bodies, updating
particles, drawing sprites. A system owns behavior, not resources; it keeps no state outside
the world except internal caches.

| Module      | Contents                                       | Stage                          |
|-------------|------------------------------------------------|--------------------------------|
| `physics`   | Rigid bodies, collision detection and response | `FixedUpdate`                  |
| `particles` | Particle emitters simulated with SIMD          | `Update`, `Render`             |
| `render`    | Sprites, camera, draw order                    | `Render`                       |

Each module provides a plugin that registers its systems in the right stage and its
settings as a resource.
