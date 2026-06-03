# RNNoise

This benchmark runs the RNNoise recurrent-neural-network noise suppression
library over a deterministic raw PCM noise input. RNNoise processes 48 kHz,
16-bit mono PCM in 480-sample frames.

The benchmark uses the official RNNoise v0.2 release tarball and compiles the
library sources directly with wasi-sdk. This avoids `autogen.sh`, autotools
cross-compilation, and runtime model-file loading; the release tarball already
contains the generated default model sources.

## Workload

`default.input` is the full `synthetic_noise.sw` file from Xiph's RNNoise data.
It is raw signed 16-bit mono PCM sampled at 48 kHz.

Source: `https://media.xiph.org/rnnoise/data/synthetic_noise.sw`

SHA-256:

`cb573a8960e22fe7a554f0ceae09f54c702ee79d8b9e94ecdb12a25c1c46273d`

Size: 38,400,000 bytes, or 40,000 RNNoise frames.

The benchmark prints the frame count and an FNV-1a checksum of the rounded
denoised output samples.

## Building

Build with Sightglass's benchmark build helper:

```sh
cd sightglass/benchmarks
./build.sh rnnoise
```

The expected checksum is intentionally left pending until the first successful
Wasm run, because this environment did not have Docker available while setting
up the build files.

## License

RNNoise is distributed under a BSD-style license. See the upstream `COPYING`
file in the RNNoise release tarball.
