add_library(ember_physics STATIC)
add_library(ember::physics ALIAS ember_physics)

target_sources(ember_physics
    PUBLIC
        FILE_SET CXX_MODULES FILES
            physics.cppm
            events.cppm
            raycast_hit.cppm
    PRIVATE
        # Partition used only inside the module. It is a module unit too, so CMake needs it in a
        # module file set.
        FILE_SET internal_partitions TYPE CXX_MODULES FILES
            src/simulation.cpp
    PRIVATE
        src/physics.cpp
)

# Box2D is private: the interfaces do not import it, only the implementation does.
target_link_libraries(ember_physics
    PUBLIC
        ember::core
    PRIVATE
        box2d_module
)

ember_enable_warnings(ember_physics)

ember_add_tests(physics
    tests/physics_test.cpp
)
