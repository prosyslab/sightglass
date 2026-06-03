cmake_minimum_required(VERSION 3.16)

project(sightglass-cvc5 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(cvc5 REQUIRED CONFIG)

add_executable(benchmark benchmark.cpp)
target_include_directories(benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(CVC5_DEP_LIB_DIR)
  target_link_directories(benchmark PRIVATE ${CVC5_DEP_LIB_DIR})
endif()
target_link_libraries(benchmark PRIVATE cvc5::cvc5 cvc5::cvc5parser)

if(EMSCRIPTEN)
  set_target_properties(benchmark PROPERTIES SUFFIX ".wasm")
  target_link_options(benchmark PRIVATE
    "SHELL:-s STANDALONE_WASM=1"
    "SHELL:-s PURE_WASI=1"
    "SHELL:-s WASMFS=1"
    "SHELL:-s ALLOW_UNIMPLEMENTED_SYSCALLS=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=0"
    "SHELL:-s INITIAL_MEMORY=536870912"
    "SHELL:-s TOTAL_STACK=8388608"
  )
endif()
