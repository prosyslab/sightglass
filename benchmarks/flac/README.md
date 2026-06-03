# FLAC

This benchmark decodes a FLAC stream with Xiph libFLAC's C stream decoder API.
FLAC is a widely used lossless audio codec.

The benchmark uses the official FLAC 1.5.0 source tag and links a small
Sightglass wrapper against `FLAC::FLAC` with wasi-sdk.

## Workload

`default.flac` was generated from a deterministic 16-bit stereo raw PCM stream
matching one of FLAC's upstream `src/test_streams/main.c` sine patterns:

```c
generate_sine16_2("sine16-13.raw", 44100.0, 200000,
                  441.0, 0.50, 4410.0, 0.49, 1.0)
```

The encoded input was produced with FLAC 1.5.0 using compression level 5,
verification enabled, and no padding.

Raw PCM SHA-256:

`db11767039104926ff50e8fc1d248bd48490fa29ea25368d795a15344f8ceb96`

FLAC SHA-256:

`293bf06ac1287d496d501360394d38b67b18f2844bf8046e57d8e6f791c39b3c`

Size: 215,423 bytes.

The benchmark reads `default.flac`, decodes it repeatedly, and prints the stream
metadata plus an FNV-1a checksum of decoded samples.

## Building

Build with Sightglass's benchmark build helper:

```sh
cd sightglass/benchmarks
./build.sh flac
```

After the first successful Sightglass run, replace the placeholder
`benchmark.stdout.expected` with the actual stdout.

## License

libFLAC is distributed under Xiph.Org's BSD-style license. See the upstream
`COPYING.Xiph` file in the FLAC source tree.
