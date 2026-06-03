cmake_minimum_required(VERSION 3.16)
project(sightglass_draco C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DRACO_ROOT "/usr/src/draco" CACHE PATH "Path to the Draco source tree")

set(DRACO_TESTS OFF CACHE BOOL "" FORCE)
set(DRACO_TRANSCODER_SUPPORTED OFF CACHE BOOL "" FORCE)
set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
set(DRACO_WASM OFF CACHE BOOL "" FORCE)
set(DRACO_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(DRACO_CXX_FLAGS "-fno-exceptions" CACHE STRING "" FORCE)

add_subdirectory("${DRACO_ROOT}" "${CMAKE_BINARY_DIR}/draco")

add_executable(benchmark benchmark.cpp)
set_target_properties(benchmark PROPERTIES SUFFIX ".wasm")
target_include_directories(benchmark PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${DRACO_ROOT}/src"
  "${CMAKE_BINARY_DIR}"
  "${CMAKE_BINARY_DIR}/draco"
)
target_link_libraries(benchmark PRIVATE draco::draco)
add_dependencies(benchmark draco_static)
target_compile_options(benchmark PRIVATE -O3 -DNDEBUG -fno-exceptions)
target_link_options(benchmark PRIVATE
  -Wl,--initial-memory=134217728
  -Wl,-z,stack-size=8388608
)
