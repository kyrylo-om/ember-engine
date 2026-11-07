# Raylib and its C++ wrapper raylib-cpp, downloaded on the first configure. raylib-cpp
# provides the C++ module `raylib` as the target raylib_cpp_modules, so code links that
# target and writes `import raylib;`. Only the platform module may use it.
#
# Raylib is built from source with the bundled GLFW, which needs the X11 and OpenGL
# development packages (see README.md).
include(FetchContent)

# raylib-cpp declares options with an old cmake_minimum_required, so option() ignores plain
# variables and the values have to be cache entries.
set(BUILD_RAYLIB_CPP_MODULES ON CACHE BOOL "Build the raylib C++ module")
set(BUILD_RAYLIB_CPP_EXAMPLES OFF CACHE BOOL "Build the raylib-cpp examples")

# raylib-cpp only adds its own targets and expects the raylib target to exist, so raylib is
# made available first. SYSTEM keeps warnings from their headers out of Ember code.
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
    URL_HASH SHA256=2b3ee1e2120c7a0796b33062c7e9a694dd8a8caa56a96319ac8c8ecf54a90d0b
    SYSTEM
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(raylib_cpp
    URL https://github.com/RobLoach/raylib-cpp/archive/refs/tags/v6.0.3.tar.gz
    URL_HASH SHA256=7eacb90823449ff4d8785807c6cd08d852492be9bf6b1d85113724e0e85f2cc4
    SYSTEM
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(raylib raylib_cpp)
