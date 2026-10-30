# Catch2 from third_party/catch2, compiled once and shared by all test executables.
# The amalgamated source already contains main(), so tests only define test cases.
add_library(ember_catch2 STATIC "${PROJECT_SOURCE_DIR}/third_party/catch2/catch_amalgamated.cpp")
target_include_directories(ember_catch2 SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/third_party/catch2")
# Plain headers only, so scanning for module imports is not needed
set_target_properties(ember_catch2 PROPERTIES CXX_SCAN_FOR_MODULES OFF)

# Adds the test executable of a module and registers it with CTest.
# Usage: ember_add_tests(<module> <test sources>...), for example ember_add_tests(core
# tests/time_test.cpp). The executable is named ember_<module>_tests and links ember::<module>.
function(ember_add_tests module)
    set(target ember_${module}_tests)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE ember::${module} ember_catch2)
    ember_enable_warnings(${target})
    add_test(NAME ${target} COMMAND ${target})
endfunction()
