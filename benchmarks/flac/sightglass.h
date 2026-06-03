#ifndef SIGHTGLASS_H
#define SIGHTGLASS_H

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((import_module("bench"), import_name("start"))) void bench_start();
__attribute__((import_module("bench"), import_name("end"))) void bench_end();

#ifdef __cplusplus
}
#endif

#endif
