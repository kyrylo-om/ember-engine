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

Requirements: Linux, GCC with C++26 support, CMake 4.2 or newer.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build
```

Warnings are treated as errors. For local experiments this can be turned off with
`-DEMBER_WARNINGS_AS_ERRORS=OFF`, but pushed code must build without warnings.

## Conventions

Read these before writing code:

- [Coding conventions](docs/conventions/coding.md)
- [Workflow and git](docs/conventions/workflow.md)
