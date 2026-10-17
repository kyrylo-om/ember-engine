add_library(ember_common STATIC)
add_library(ember::common ALIAS ember_common)

target_sources(ember_common
    PUBLIC
        FILE_SET CXX_MODULES FILES
            common.cppm
            math.cppm
            handle.cppm
            reflect.cppm
)

ember_enable_warnings(ember_common)
