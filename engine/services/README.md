# Services

Modules that provide capabilities on demand: opening a window, loading an asset, showing
the debug overlay. A service owns resources with a lifetime (the window, loaded textures,
the console thread), is stored as an ECS resource and is called by systems or other
services.

| Module     | Contents                                                        |
|------------|-----------------------------------------------------------------|
| `platform` | Window, drawing, audio, input; the only module that uses Raylib |
| `assets`   | Embedded asset catalog, asset manager, scene loading and saving |
| `debug`    | Debug overlay, console, inspector                               |

Services are created in the order `platform`, `assets`, `debug` and destroyed in reverse.
