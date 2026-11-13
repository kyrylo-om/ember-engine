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
        src/input.cpp
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
    tests/input_test.cpp
)
