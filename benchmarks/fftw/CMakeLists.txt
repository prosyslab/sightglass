cmake_minimum_required(VERSION 3.16)
project(sightglass_fftw C)

set(FFTW_PREFIX "/usr/local/fftw" CACHE PATH "Path to the FFTW install prefix")

add_executable(benchmark benchmark.c)
set_target_properties(benchmark PROPERTIES SUFFIX ".wasm")
target_include_directories(benchmark PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${FFTW_PREFIX}/include"
)
target_link_libraries(benchmark PRIVATE
  "${FFTW_PREFIX}/lib/libfftw3.a"
  m
)
target_compile_options(benchmark PRIVATE -O3 -DNDEBUG)
target_link_options(benchmark PRIVATE
  -Wl,--initial-memory=134217728
  -Wl,-z,stack-size=8388608
)
