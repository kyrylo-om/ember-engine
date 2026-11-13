add_library(ember_platform STATIC)
add_library(ember::platform ALIAS ember_platform)

target_sources(ember_platform
    PUBLIC
        FILE_SET CXX_MODULES FILES
            platform.cppm
            audio.cppm
            graphics.cppm
            image.cppm
            input.cppm
            platform_error.cppm
            window.cppm
    PRIVATE
        # Partitions used only inside the module. They are module units too, so CMake needs
        # them in a module file set.
        FILE_SET internal_partitions TYPE CXX_MODULES FILES
            src/raylib_support.cpp
            src/resource_table.cpp
    PRIVATE
        src/audio.cpp
        src/graphics.cpp
        src/image.cpp
        src/input.cpp
        src/platform.cpp
        src/window.cpp
)

# Raylib is private: the interfaces do not import it, only the implementation does.
target_link_libraries(ember_platform
    PUBLIC
        ember::core
    PRIVATE
        raylib_cpp_modules
)

ember_enable_warnings(ember_platform)

ember_add_tests(platform
    tests/audio_test.cpp
    tests/graphics_test.cpp
    tests/image_test.cpp
    tests/input_test.cpp
    tests/platform_test.cpp
)
