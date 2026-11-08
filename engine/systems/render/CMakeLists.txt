add_library(ember_render STATIC)
add_library(ember::render ALIAS ember_render)

target_sources(ember_render
    PUBLIC
        FILE_SET CXX_MODULES FILES
            render.cppm
            active_camera.cppm
            culling.cppm
            shape_queue.cppm
)

target_link_libraries(ember_render PUBLIC ember::platform)

ember_enable_warnings(ember_render)
