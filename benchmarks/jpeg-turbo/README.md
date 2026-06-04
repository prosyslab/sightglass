# libjpeg-turbo

This benchmark decodes and encodes a JPEG image with libjpeg-turbo's TurboJPEG
C API. libjpeg-turbo is a widely used JPEG codec library and is the upstream
library behind Phoronix's `tjbench` profile.

The benchmark uses libjpeg-turbo `3.1.4.1`, pinned to commit
`9217719d3a58633923b096af4c1d50d304768a64`, and links a small Sightglass
wrapper against a static WASI build of `libturbojpeg`.

It uses wasi-sdk 33 because libjpeg-turbo's TurboJPEG API uses
`setjmp`/`longjmp` for error recovery. wasi-sdk documents this as supported
with `-mllvm -wasm-enable-sjlj -lsetjmp -mllvm -wasm-use-legacy-eh=false`.

## Workload

`default.jpg` is a 640 by 427 baseline JPEG image already present in the
Sightglass image-classification benchmark inputs.

Input SHA-256:

`58872621bb61bfa99670157e7402c7d59003d532fb5f78960a6e1177c202dc17`

The timed region performs 128 iterations. Each iteration decodes the input JPEG
to RGB, encodes the RGB buffer back to JPEG at quality 85 with 4:2:0
subsampling, decodes the re-encoded JPEG, and folds both the encoded JPEG bytes
and final RGB bytes into an FNV-1a checksum.

## Building

Build with Sightglass's benchmark build helper:

```sh
cd sightglass/benchmarks
./build.sh jpeg-turbo
```

After the first successful Sightglass run, replace the placeholder
`benchmark.stdout.expected` with the actual stdout.

Run this benchmark with Wasm exceptions enabled, since wasi-sdk's
setjmp/longjmp lowering emits exception-handling instructions:

```sh
cargo run --release -- benchmark \
  --engine-flags "-W exceptions=y" \
  benchmarks/jpeg-turbo/benchmark.wasm \
  -e path/to/libwasmtime_bench_api.so
```

## License

libjpeg-turbo is distributed under a BSD-style license. See the upstream
`LICENSE.md` file for details.
