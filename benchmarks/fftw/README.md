# FFTW

This benchmark runs FFTW's public C API over a fixed mix of one-dimensional and
two-dimensional complex DFTs. FFTW is a widely used Fourier transform library
used in signal processing, scientific computing, image processing, and audio
workloads.

The benchmark uses the official FFTW 3.3.11 source tarball and links a small
Sightglass wrapper against a static WASI build of `libfftw3`.

## Workload

The workload is intentionally Phoronix-shaped without using FFTW's internal
`tests/bench` driver. Sightglass owns timing through `bench_start()` and
`bench_end()`.

`default.input` defines:

* 128 iterations.
* One complex 1D forward DFT of size 4096.
* One complex 2D forward DFT of size 512 by 512.
* A deterministic seed for generating input values.

Plans are created before `bench_start()` with `FFTW_MEASURE`. The timed region
executes the plans repeatedly, and the benchmark prints the workload metadata
and an FNV-1a checksum of the final output arrays.

## Building

Build with Sightglass's benchmark build helper:

```sh
cd sightglass/benchmarks
./build.sh fftw
```

After the first successful Sightglass run, replace the placeholder
`benchmark.stdout.expected` with the actual stdout.

## License

FFTW is distributed under the GNU General Public License. See the upstream FFTW
license documentation for details.
