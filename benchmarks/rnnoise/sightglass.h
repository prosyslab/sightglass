#ifndef sightglass_h
#define sightglass_h 1

__attribute__((import_module("bench")))
__attribute__((import_name("start")))
void bench_start();

__attribute__((import_module("bench")))
__attribute__((import_name("end")))
void bench_end();

#endif
