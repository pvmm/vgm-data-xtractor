cmake_minimum_required(VERSION 3.15)
project(BaseProject)

function(configure_target)
	set(TARGET "${ARGV0}")
	list(REMOVE_AT ARGV 0)
	execute_process(COMMAND cmake -B build -S . ${ARGV}
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}"
		COMMAND_ERROR_IS_FATAL ANY)
	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}/build)
		message(FATAL_ERROR "directory ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/build not found")
	endif()
endfunction()

# Configure and build projects
function(build_target)
	configure_target(${ARGV})
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

#
# Configure and build project
#

if(PLATFORM STREQUAL Web)
	set(USE_EMCC ON)
endif()

if(DEFINED USE_EMCC)
	# Install emsdk and update EMROOT and PATH
	build_target(emsdk)
	set(ENV{EMROOT} ${CMAKE_CURRENT_SOURCE_DIR}/emsdk/build/emsdk/src/emsdk/upstream/emscripten)
	set(ENV{PATH} "$ENV{PATH}:$ENV{EMROOT}")
	set(CMAKE_PROGRAM_PATH $ENV{EMROOT})
	message("-- Added $ENV{EMROOT} to PATH")

	# check if emcc works
	execute_process(COMMAND emcc --version OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)

	# Configure vgmdata project with Emscripten
	build_emcc_target(vgmdata
		-DPLATFORM=${PLATFORM}
		-DCMAKE_BUILD_TYPE=DEBUG)
else()
	find_program(WASM-LD_EXE NAMES wasm-ld HINTS $ENV{EMROOT}/../bin REQUIRED)
	if(WASM-LD_EXE STREQUAL "WASM-LD_EXE-NOTFOUND")
		message(FATAL_ERROR "wasm-ld linker not found")
	else()
		message("-- Linker set to ${WASM-LD_EXE}")
	endif()
	find_program(CLANG_EXE NAMES clang HINTS $ENV{EMROOT}/../bin REQUIRED)
	if(CLANG_EXE STREQUAL "CLANG_EXE-NOTFOUND")
		message(FATAL_ERROR "emsdk clang not found")
	else()
		message("-- Compiler set to ${CLANG_EXE}")
	endif()
	message("-- Checking if clang works... ${CLANG_EXE}")
	execute_process(COMMAND ${CLANG_EXE} --version OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
	message("-- Found clang executable at ${CLANG_EXE}")

	# Configure vgmdata project with Emscripten
	build_target(vgmdata
		-DPLATFORM=${PLATFORM}
		-DCMAKE_BUILD_TYPE=DEBUG)
endif()
