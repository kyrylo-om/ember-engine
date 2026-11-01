add_library(ember_core STATIC)
add_library(ember::core ALIAS ember_core)

target_sources(ember_core
    PUBLIC
        FILE_SET CXX_MODULES FILES
            core.cppm
            application.cppm
            event_bus.cppm
            log.cppm
            scheduler.cppm
            time.cppm
    PRIVATE
        src/event_bus.cpp
        src/log.cpp
        src/time.cpp
)

target_link_libraries(ember_core PUBLIC ember::ecs)

ember_enable_warnings(ember_core)

ember_add_tests(core
    tests/event_bus_test.cpp
    tests/log_test.cpp
    tests/time_test.cpp
)
