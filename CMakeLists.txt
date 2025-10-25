# Minimum CMake version
cmake_minimum_required(VERSION 3.14)

# Project name and languages
project(raylib_node2d LANGUAGES C CXX)

# Set C++ standard
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Download CPM.cmake for dependency management
# file(DOWNLOAD
#     "https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake"
#     "${CMAKE_BINARY_DIR}/CPM_0.42.0.cmake"
# )
# include(${CMAKE_BINARY_DIR}/CPM_0.42.0.cmake)

# Include FetchContent module for raylib
include(FetchContent)
#================================================
# Fetch raylib
#================================================
# Declare raylib 5.5 from GitHub
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)
#================================================
# raygui
#================================================
FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 4.0
    GIT_SHALLOW TRUE
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(raygui)
#================================================
# FLECS
#================================================
FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG v4.1.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(flecs)

set(APP_NAME raylib_node2d)
add_executable(${APP_NAME}
    # src/main.c
    # src/main_flecs.c
    src/main_flecs_node2d.c
    # src/raylib2d_flecs_transform_hierarchy04.c
    # src/raylib2d_flecs_transform_hierarchy05.c
    # src/raylib_jolt_contact.c
    # src/raylib_jolt_controller.c
)
target_compile_definitions(${APP_NAME} PRIVATE 
    RAYGUI_IMPLEMENTATION=1
)
# Link libraries (raylib and joltc)
target_link_libraries(${APP_NAME} PRIVATE
    raylib
    flecs                                           # flecs
    # joltc
)
# Include headers for raylib and joltc
target_include_directories(${APP_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include                       # include
    ${raylib_SOURCE_DIR}/src
    ${raygui_SOURCE_DIR}/src                            # raygui
    ${flecs_SOURCE_DIR}/include                         # flecs
    # ${joltc_SOURCE_DIR}/include
)
# Platform-specific settings
if(WIN32)
    target_link_libraries(${APP_NAME} PRIVATE 
        ws2_32 
        winmm
    )
    target_link_options(${APP_NAME} PRIVATE
        -static-libgcc
        -static-libstdc++  # Uncomment if C++ code is used
        -static           # Avoid full static linking to prevent issues with system libraries
    )
endif()