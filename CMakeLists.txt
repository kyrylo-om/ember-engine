cmake_minimum_required(VERSION 4.2)

# Enables `import std;`, which CMake still marks as experimental. The value is tied to the
# CMake version and has to be updated together with cmake_minimum_required.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Standard C++ only, without compiler extensions, so the code stays portable
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_MODULE_STD ON)

project(ember
    VERSION 0.1.0
    DESCRIPTION "Minimalistic 2D game framework for modern C++"
    LANGUAGES CXX
)

if(NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio")
    message(FATAL_ERROR "C++ modules need the Ninja or Visual Studio generator, "
        "not ${CMAKE_GENERATOR}. Configure with -G Ninja.")
endif()

# Used by clangd and other tools for code navigation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EMBER_WARNINGS_AS_ERRORS "Treat warnings in Ember code as errors" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(compiler_warnings)

enable_testing()
