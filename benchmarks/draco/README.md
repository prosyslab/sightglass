# Draco

This benchmark decodes a Draco-compressed triangular mesh with Google Draco's
C++ decoder. Draco is a 3D geometry compression library used for transmission
and loading of compressed meshes in client Web and game-style workloads.

The benchmark uses the official Draco 1.5.7 source tag and links a small
Sightglass wrapper against `draco::draco` with wasi-sdk. It does not use
Draco's Emscripten JavaScript/Wasm wrapper build.

## Workload

`default.drc` is copied from Draco's upstream `testdata/bunny_gltf.drc`.

Source: `https://github.com/google/draco/blob/1.5.7/testdata/bunny_gltf.drc`

SHA-256:

`c1bf564f279849f311c4b4205dd16d729557c208fd9502ab124b784d146add80`

Size: 120,867 bytes.

The benchmark decodes this mesh repeatedly and prints a checksum of the decoded
faces and attribute data.

## Building

Build with Sightglass's benchmark build helper:

```sh
cd sightglass/benchmarks
./build.sh draco
```

The expected stdout records the decoded mesh metadata and checksum.

## License

Draco is distributed under the Apache License 2.0. See the upstream `LICENSE`
file in the Draco source tree.
