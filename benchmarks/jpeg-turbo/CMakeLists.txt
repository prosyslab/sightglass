cmake_minimum_required(VERSION 3.16)
project(sightglass_jpeg_turbo C)

set(JPEG_TURBO_PREFIX "/usr/local/jpeg-turbo" CACHE PATH
  "Path to the libjpeg-turbo install prefix")

add_executable(benchmark benchmark.c)
set_target_properties(benchmark PROPERTIES SUFFIX ".wasm")
target_include_directories(benchmark PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${JPEG_TURBO_PREFIX}/include"
)
target_link_libraries(benchmark PRIVATE
  "${JPEG_TURBO_PREFIX}/lib/libturbojpeg.a"
  m
)
target_compile_options(benchmark PRIVATE -O3 -DNDEBUG)
target_link_libraries(benchmark PRIVATE setjmp)
target_compile_options(benchmark PRIVATE
  "SHELL:-mllvm -wasm-enable-sjlj -mllvm -wasm-use-legacy-eh=false"
)
target_link_options(benchmark PRIVATE
  -Wl,-mllvm,-wasm-enable-sjlj,-mllvm,-wasm-use-legacy-eh=false
  -Wl,--initial-memory=134217728
  -Wl,-z,stack-size=8388608
)
