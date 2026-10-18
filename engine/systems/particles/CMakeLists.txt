add_library(ember_particles STATIC)
add_library(ember::particles ALIAS ember_particles)

target_sources(ember_particles
    PUBLIC
        FILE_SET CXX_MODULES FILES
            particles.cppm
)

target_link_libraries(ember_particles PUBLIC ember::core)

ember_enable_warnings(ember_particles)
