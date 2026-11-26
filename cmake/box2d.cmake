# Box2D, the 2D physics engine behind the physics module, downloaded on the first configure.
# Box2D is a C library, so box2d/box2d.cppm wraps its header in the C++ module `box2d`, built
# as the target box2d_module. Code links that target and writes `import box2d;`. Only physics
# uses it.
#
# Box2D hides its samples, tests and benchmarks when it is not the top-level project, so only
# the library itself is built.
include(FetchContent)

# SYSTEM keeps warnings from its headers out of Ember code.
FetchContent_Declare(box2d
    URL https://github.com/erincatto/box2d/archive/refs/tags/v3.2.0.tar.gz
    URL_HASH SHA256=ffec747b27a2267016f8d0240d658c82d187fb2271fa0f69fcbe53fbc067180d
    SYSTEM
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(box2d)

add_library(box2d_module STATIC)
target_sources(box2d_module
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}/box2d"
        FILES "${CMAKE_CURRENT_LIST_DIR}/box2d/box2d.cppm"
)
target_link_libraries(box2d_module PUBLIC box2d::box2d)
ember_enable_warnings(box2d_module)
