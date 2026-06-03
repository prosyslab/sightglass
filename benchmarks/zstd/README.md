# Zstandard

This benchmark compresses and decompresses a Silesia corpus input with the
upstream Zstandard C library. Zstandard is a widely used lossless compression
algorithm and library from Meta.

The benchmark uses zstd `v1.5.7`, pinned to commit
`f8745da6ff1ad1e7bab384bd1f9d742439278e99`, and links a small Sightglass
wrapper against a static WASI build of `libzstd`.

## Workload

`default.input` is the Silesia corpus `samba` member: a tarred Samba 2-2.3
source tree with mixed source code and graphics. The Silesia corpus was created
as a compression benchmark corpus whose files cover larger modern data types
than older Calgary and Canterbury corpora.

Expected input metadata:

* Source: Silesia corpus `samba`
* Size: 21,606,400 bytes
* MD5: `154eaea7ea70e89f6339ff0abf4112ca`
* SHA-256: `93ba07bc44d8267789c1d911992f40b089ffa2140b4a160fac11ccae9a40e7b2`

The timed region performs 4 compression/decompression round trips at zstd
compression level 3 and verifies that each decompressed buffer matches the
original input. The benchmark prints the final compressed size and an FNV-1a
checksum over compressed and decompressed bytes.

## Building

Build with Sightglass's benchmark build helper:

```sh
cd sightglass/benchmarks
./build.sh zstd
```

After the first successful Sightglass run, replace the placeholder
`benchmark.stdout.expected` with the actual stdout.

## License

Zstandard is distributed under the BSD 3-Clause license. The Silesia corpus is a
public benchmark corpus; see the upstream Silesia page for corpus details.
