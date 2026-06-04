#pragma once

__attribute__((import_module("bench"), import_name("start"))) void
bench_start(void);
__attribute__((import_module("bench"), import_name("end"))) void bench_end(void);
