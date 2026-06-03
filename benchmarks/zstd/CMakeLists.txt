cmake_minimum_required(VERSION 3.16)
project(sightglass_zstd C)

set(ZSTD_ROOT "/usr/src/zstd" CACHE PATH "Path to the zstd source tree")

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)
set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL "" FORCE)

add_subdirectory("${ZSTD_ROOT}/build/cmake" "${CMAKE_BINARY_DIR}/zstd")

add_executable(benchmark benchmark.c)
set_target_properties(benchmark PROPERTIES SUFFIX ".wasm")
target_include_directories(benchmark PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${ZSTD_ROOT}/lib"
)
target_link_libraries(benchmark PRIVATE libzstd_static)
add_dependencies(benchmark libzstd_static)
target_compile_options(benchmark PRIVATE -O3 -DNDEBUG)
target_link_options(benchmark PRIVATE
  -Wl,--initial-memory=134217728
  -Wl,-z,stack-size=8388608
)
