cmake_minimum_required(VERSION 3.15)
project(EmscriptenSetup)

# Directory for installing emsdk
set(EMSDK_DIR "${CMAKE_BINARY_DIR}/emsdk/src/emsdk" CACHE PATH "Directory to install emsdk")

# Define a target to download and install emsdk
include(ExternalProject)

function(setupEmsdk)
    ExternalProject_Add (emsdk
        GIT_REPOSITORY https://github.com/emscripten-core/emsdk
	GIT_TAG main
        PREFIX "${PROJECT_BINARY_DIR}/emsdk"
        CONFIGURE_COMMAND ${EMSDK_DIR}/emsdk install latest
        BUILD_COMMAND ${EMSDK_DIR}/emsdk activate latest
        INSTALL_COMMAND "" # source ${EMSDK_DIR}/emsdk_env.sh
    )
endfunction()

setupEmsdk()

add_custom_command(
    TARGET emsdk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "-- Emscripten installation finished")
