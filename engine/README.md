# Engine

The Ember library, organized in layers. Each module is a separate CMake target and C++
module with its own interfaces, implementation and tests.

```text
engine/
├── common/          shared types, components, events, reflection helpers (interface-only)
├── ecs/             entities, components, views, resources, commands
├── core/            application, main loop, time, scheduler, event bus, logging
├── services/        modules that own resources and are called by other code
│   ├── platform/    window, drawing, audio, input; the only module that uses Raylib
│   ├── assets/      embedded asset catalog, asset manager, scenes
│   └── debug/       debug overlay, console, inspector
└── systems/         modules that run every frame over components
    ├── physics/     rigid bodies, collision detection and response
    ├── particles/   particle emitters simulated with SIMD
    └── render/      sprites, camera, draw order
```

## Dependencies

| Module                                   | Depends on         |
|------------------------------------------|--------------------|
| `common`                                 | -                  |
| `ecs`                                    | `common`           |
| `core`                                   | `ecs`              |
| `platform`, `physics`                    | `core`             |
| `assets`, `debug`, `particles`, `render` | `core`, `platform` |

A module also depends on everything its dependencies depend on.

- A module imports only the modules it depends on.
- Lower layers know nothing about higher ones: `common`, then `ecs`, then `core`, then
  services and systems, then games.
- Services and systems do not depend on each other, except on `platform` for drawing,
  audio and GPU resources.
- Raylib types never appear in exported declarations.

## Module layout

Every module folder has the same layout:

```text
<module>/
├── CMakeLists.txt
├── README.md
├── <module>.cppm             primary module interface
├── <partition>.cppm          interface partitions
├── src/                      implementation units
└── tests/                    unit tests
```

The `services/` and `systems/` groups only exist in the folder structure. Everywhere else a
module is called by its own name: for `engine/systems/physics/` the CMake target is
`ember::physics`, the namespace `ember::physics`, the C++ module `ember.physics` and the
commit scope `physics`.

To add a module: create its folder with this layout in the right group, add it to the
CMake build, call `ember_enable_warnings()` on its compiled targets, then add it to this
README, to the group README and to the scopes in `docs/conventions/workflow.md`.
