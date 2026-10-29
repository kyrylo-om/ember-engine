add_library(ember_ecs STATIC)
add_library(ember::ecs ALIAS ember_ecs)

target_sources(ember_ecs
    PUBLIC
        FILE_SET CXX_MODULES FILES
            ecs.cppm
            commands.cppm
            entity.cppm
            registry.cppm
            view.cppm
            world.cppm
            components/components.cppm
            components/camera.cppm
            components/collider.cppm
            components/emitter.cppm
            components/name.cppm
            components/rigid_body.cppm
            components/sprite.cppm
            components/transform.cppm
            components/velocity.cppm
)

target_link_libraries(ember_ecs PUBLIC ember::common)

ember_enable_warnings(ember_ecs)
