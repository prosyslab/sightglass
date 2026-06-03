cmake_minimum_required(VERSION 3.16)
project(sightglass_flac C)

set(FLAC_ROOT "/usr/src/flac" CACHE PATH "Path to the FLAC source tree")

set(BUILD_CXXLIBS OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(WITH_OGG OFF CACHE BOOL "" FORCE)
set(WITH_ASM OFF CACHE BOOL "" FORCE)
set(ENABLE_MULTITHREADING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(WITH_STACK_PROTECTOR OFF CACHE BOOL "" FORCE)
set(WITH_FORTIFY_SOURCE OFF CACHE BOOL "" FORCE)
set(INSTALL_MANPAGES OFF CACHE BOOL "" FORCE)
set(INSTALL_PKGCONFIG_MODULES OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)

add_subdirectory("${FLAC_ROOT}" "${CMAKE_BINARY_DIR}/flac")

target_compile_definitions(FLAC PRIVATE
  chmod=flac_wasi_chmod
  chown=flac_wasi_chown
)
target_compile_options(FLAC PRIVATE
  -include "${CMAKE_CURRENT_SOURCE_DIR}/flac_wasi_compat.h"
)

add_executable(benchmark benchmark.c flac_wasi_compat.c)
set_target_properties(benchmark PROPERTIES SUFFIX ".wasm")
target_include_directories(benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(benchmark PRIVATE FLAC::FLAC)
add_dependencies(benchmark FLAC)
target_compile_options(benchmark PRIVATE -O3 -DNDEBUG)
target_link_options(benchmark PRIVATE
  -Wl,--initial-memory=134217728
  -Wl,-z,stack-size=8388608
)
