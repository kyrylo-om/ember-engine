add_executable(yozakura main.cpp)

target_sources(yozakura
    PRIVATE
        FILE_SET CXX_MODULES FILES
            art.cppm
            sound.cppm
)

target_link_libraries(yozakura PRIVATE
    ember::core
    ember::particles
    ember::physics
    ember::platform
)

ember_enable_warnings(yozakura)
