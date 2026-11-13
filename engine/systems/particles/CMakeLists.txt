add_library(ember_particles STATIC)
add_library(ember::particles ALIAS ember_particles)

target_sources(ember_particles
    PUBLIC
        FILE_SET CXX_MODULES FILES
            particles.cppm
            particle_pool.cppm
            particle_spawner.cppm
    PRIVATE
        src/particle_pool.cpp
        src/particle_spawner.cpp
        src/particles.cpp
)

target_link_libraries(ember_particles PUBLIC ember::core)

ember_enable_warnings(ember_particles)

ember_add_tests(particles
    tests/particle_pool_test.cpp
    tests/particle_spawner_test.cpp
    tests/particles_test.cpp
)
