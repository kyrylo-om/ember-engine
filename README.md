# Ember

Ember is a minimalistic 2D game framework written in modern C++ (C++26). A game is an
ordinary C++ program: it links Ember, configures an application, registers its systems
and runs. Windowing, graphics, audio and input are provided by Raylib.

## Repository layout

| Path           | Contents                                                     |
|----------------|--------------------------------------------------------------|
| `engine/`      | The engine library, one folder per module                    |
| `examples/`    | Programs built with Ember: the sample game and small demos   |
| `cmake/`       | CMake helper scripts                                         |
| `docs/`        | Conventions and design documents                             |
| `third_party/` | External libraries stored in the repository                  |

Most folders have a short README that explains what belongs there.

## Building

Requirements: Linux, GCC 16 (`g++-16`), CMake 4.2, Ninja, and the X11 and OpenGL
development packages that Raylib is built with. On Ubuntu:

```sh
sudo apt install ninja-build libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libxi-dev libgl1-mesa-dev
```

Raylib and raylib-cpp are downloaded by CMake on the first configure, so that step needs
network access.

Configure, build and run the tests in one step:

```sh
cmake --workflow --preset debug
```

Or step by step:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

The `release` preset builds with optimizations. Build output goes to `build/<preset>/`.
Editors with CMake support, such as VS Code with CMake Tools, offer the same presets.
Personal presets, for example with another compiler, go into `CMakeUserPresets.json`,
which is not committed.

Warnings are treated as errors. For local experiments this can be turned off with
`cmake --preset debug -DEMBER_WARNINGS_AS_ERRORS=OFF`, but pushed code must build without
warnings.

## Conventions

Read these before writing code:

- [Coding conventions](docs/conventions/coding.md)
- [Workflow and git](docs/conventions/workflow.md)
