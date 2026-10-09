cmake_minimum_required(VERSION 3.30)

project(ember
    VERSION 0.1.0
    DESCRIPTION "Minimalistic 2D game framework for modern C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Standard C++ only, without compiler extensions, so the code stays portable
set(CMAKE_CXX_EXTENSIONS OFF)
# Used by clangd and other tools for code navigation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EMBER_WARNINGS_AS_ERRORS "Treat warnings in Ember code as errors" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(compiler_warnings)

enable_testing()
