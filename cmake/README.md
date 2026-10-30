# CMake

Scripts included by the root `CMakeLists.txt`.

| File                      | Contents                                                                |
|---------------------------|-------------------------------------------------------------------------|
| `compiler_warnings.cmake` | `ember_enable_warnings(<target>)`: warning flags for every Ember target |
| `catch2.cmake`            | Catch2 library; `ember_add_tests(<module> <sources>...)`: module tests   |

Dependency setup and other shared CMake functions belong here as well. Third-party
libraries are added as `SYSTEM`, so warnings from their headers do not fail our build.
