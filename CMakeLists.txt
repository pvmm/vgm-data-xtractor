cmake_minimum_required(VERSION 3.15)
project(BaseProject)

function(configure_target)
	execute_process(COMMAND cmake -B build -S .
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}"
		COMMAND_ERROR_IS_FATAL ANY)
	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}/build)
		message(FATAL_ERROR "directory ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}/build not found")
	endif()
endfunction()

# Configure and build project
function(build_target)
	configure_target(${ARGV0})
	execute_process(COMMAND make
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}/build"
		COMMAND_ERROR_IS_FATAL ANY)
endfunction()

# Configure and build project with emsdk
function(build_emcc_target)
	if(${ARGC} EQUAL 0)
		message(FATAL_ERROR "missing argument: target name")
	endif()
	# Remove first parameter and call it TARGET
	set(TARGET "${ARGV0}")
	list(REMOVE_AT ARGV 0)
	execute_process(
		COMMAND ${CMAKE_COMMAND} -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_TOOLCHAIN_FILE=$ENV{EMROOT}/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_C_COMPILER=emcc ${ARGV} -B build -S .
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}"
		COMMAND_ERROR_IS_FATAL ANY)
	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/build)
		message(FATAL_ERROR "directory ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/build not found")
	endif()
	execute_process(COMMAND make
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/build"
		COMMAND_ERROR_IS_FATAL ANY)
endfunction()

function(config_clang_target)
	if(${ARGC} EQUAL 0)
		message(FATAL_ERROR "missing argument: target name")
	endif()
	message("CLANG compiler is ${CLANG_EXE}")
	# Remove first parameter and call it TARGET
	set(TARGET "${ARGV0}")
	list(REMOVE_AT ARGV 0)
	execute_process(
		COMMAND ${CMAKE_COMMAND} -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_C_COMPILER=${CLANG_EXE} ${ARGV} -B build -S . --trace
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}"
		COMMAND_ERROR_IS_FATAL ANY)
	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/build)
		message(FATAL_ERROR "directory ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/build not found")
	endif()
endfunction()

# Configure and build project

# Install emsdk and update EMROOT and PATH
build_target(emsdk)
set(ENV{EMROOT} ${CMAKE_CURRENT_SOURCE_DIR}/emsdk/build/emsdk/src/emsdk/upstream/emscripten)
set(ENV{PATH} "$ENV{PATH}:$ENV{EMROOT}")
set(CMAKE_PROGRAM_PATH $ENV{EMROOT})
message("-- Added $ENV{EMROOT} to PATH")

# Define clang executables
if(DEFINED USE_CLANG)
	#find_program(WASM-LD_EXE NAME wasm-ld HINTS $ENV{EMROOT}/../bin REQUIRED)
	set(WASM-LD_EXE ${CMAKE_CURRENT_SOURCE_DIR}/emsdk/build/emsdk/src/emsdk/upstream/bin/wasm-ld)
	if(WASM-LD_EXE STREQUAL "WASM-LD_EXE-NOTFOUND")
		message(FATAL_ERROR "wasm-ld linker not found")
	else()
		message("-- Linker set to ${WASM-LD_EXE}")
	endif()
	#find_program(CLANG_EXE NAME clang HINTS $ENV{EMROOT}/../bin REQUIRED)
	set(CLANG_EXE ${CMAKE_CURRENT_SOURCE_DIR}/emsdk/build/emsdk/src/emsdk/upstream/bin/clang)
	if(CLANG_EXE STREQUAL "CLANG_EXE-NOTFOUND")
		message(FATAL_ERROR "emsdk clang not found")
	else()
		message("-- Compiler set to ${CLANG_EXE}")
	endif()
	message("-- Checking if clang works... ${CLANG_EXE}")
	execute_process(COMMAND ${CLANG_EXE} --version OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
	message("-- Found clang executable at ${CLANG_EXE}")
endif()

# check if emcc works
execute_process(COMMAND emcc --version OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)

# Configure and build raylib
build_emcc_target(raylib -DCMAKE_BUILD_TYPE=DEBUG)

# Get raylib options back
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/raylib/build/RAYLIB_SRC.txt" RAYLIB_SRC)
set(ENV{INCLUDE_DIRECTORIES} ${RAYLIB_SRC})
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/raylib/build/RAYLIB_BIN.txt" RAYLIB_BIN)
set(ENV{INCLUDE_DIRECTORIES} ${RAYLIB_BIN})

# Configure vgmdata project with Emscripten
build_emcc_target(vgmdata -DRAYLIB_SRC=${RAYLIB_SRC} -DRAYLIB_BIN=${RAYLIB_BIN})
