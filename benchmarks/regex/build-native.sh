#!/usr/bin/env bash

# Build regex benchmark as native shared libraries (Linux-only).
#
# Usage: ./build-native.sh

(set -x;)
(rm -rf rust-benchmark-native);
(cp -r rust-benchmark rust-benchmark-native/);
(cp sightglass.native.patch rust-benchmark-native/);
(cd rust-benchmark-native; patch -Np1 -i ./sightglass.native.patch; cd -);
(cd rust-benchmark-native; cargo build --release --target-dir target-llvm-release; cp target-llvm-release/release/libbenchmark.so ../benchmark.llvm.release.so; cd -);
(cd rust-benchmark-native; cargo build --target-dir target-llvm-debug; cp target-llvm-debug/debug/libbenchmark.so ../benchmark.llvm.debug.so; cd -);
(set +x;)
