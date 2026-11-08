add_library(ember_debug STATIC)
add_library(ember::debug ALIAS ember_debug)

target_sources(ember_debug
    PUBLIC
        FILE_SET CXX_MODULES FILES
            debug.cppm
            console.cppm
            debug_overlay.cppm
            inspector.cppm
)

target_link_libraries(ember_debug PUBLIC ember::platform)

ember_enable_warnings(ember_debug)
