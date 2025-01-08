cmake_minimum_required(VERSION 3.27)
project(raylib C)
set(CMAKE_VERBOSE_MAKEFILE ON)
include(FetchContent)
include(ExternalProject)
set(FETCHCONTENT_QUIET FALSE)

# Define the PLATFORM option
set(PLATFORM "Web" CACHE STRING "Target platform for compilation: Web or Desktop")

# Supported platforms
set(SUPPORTED_PLATFORMS "Web" "Desktop") # "Android" "Raspberry")

# Check if the platform is valid
list(FIND SUPPORTED_PLATFORMS "${PLATFORM}" PLATFORM_INDEX)
if(PLATFORM_INDEX EQUAL -1)
    message(FATAL_ERROR "Invalid PLATFORM: ${PLATFORM}. Supported values are: ${SUPPORTED_PLATFORMS}")
endif()

message("-- [raylib] C compiler is ${CMAKE_C_COMPILER}")

# Adding raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

# Configure platform-specific settings
if(PLATFORM STREQUAL "Web")
    message("-- Configuring for Web platform...")
    if(NOT EXISTS $ENV{EMROOT})
        message(FATAL_ERROR "EMROOT environment variable not found")
    endif()
    set(ENV{PATH} "$ENV{PATH}:${EMROOT}")
    set_target_properties(raylib PROPERTIES CMAKE_VERBOSE_MAKEFILE ON
        CMAKE_TOOLCHAIN_FILE ${EMROOT}/cmake/Modules/Platform/Emscripten.cmake
        CMAKE_C_COMPILER emcc)
elseif(PLATFORM STREQUAL "Desktop")
    # Desktop-specific configurations
    message("-- Configuring for Desktop platform...")
    add_compile_definitions(PLATFORM_DESKTOP)
    set_target_properties(raylib PROPERTIES CMAKE_VERBOSE_MAKEFILE ON)
endif()

# Get path to raylib.h
file(GLOB_RECURSE raylib_h ${raylib_SOURCE_DIR} raylib.h)
list(GET raylib_h 0 raylib_h)
get_filename_component(RAYLIB_SRC ${raylib_h} DIRECTORY)
message("-- Set RAYLIB_SRC to ${RAYLIB_SRC}")
file(WRITE "${CMAKE_BINARY_DIR}/RAYLIB_SRC.txt" "${RAYLIB_SRC}")

# Get path to libraylib.a
file(GLOB_RECURSE raylib_a ${raylib_BINARY_DIR} libraylib.a)
list(GET raylib_a 0 raylib_a)
get_filename_component(RAYLIB_BIN ${raylib_a} DIRECTORY)
message("-- Set RAYLIB_BIN to ${RAYLIB_BIN}")
file(WRITE "${CMAKE_BINARY_DIR}/RAYLIB_BIN.txt" "${RAYLIB_BIN}")
