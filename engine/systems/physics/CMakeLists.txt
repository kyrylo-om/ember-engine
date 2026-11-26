add_library(ember_physics STATIC)
add_library(ember::physics ALIAS ember_physics)

target_sources(ember_physics
    PUBLIC
        FILE_SET CXX_MODULES FILES
            physics.cppm
            events.cppm
)

target_link_libraries(ember_physics PUBLIC ember::core)

ember_enable_warnings(ember_physics)
