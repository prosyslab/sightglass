#!/usr/bin/env bash
#
# Build this benchmark as a native shared library (benchmark.so).
#
set -euo pipefail
(set -x;)

# Some benchmarks require setup to fetch model/assets.
if [[ -f ./setup.sh ]]; then
  bash ./setup.sh
fi

rm -rf rust-benchmark-native
cp -r rust-benchmark rust-benchmark-native/
cp sightglass.native.patch rust-benchmark-native/
(cd rust-benchmark-native; patch -Np1 -i ./sightglass.native.patch; cd -)

(
  cd rust-benchmark-native
  cargo build --release --target-dir target-llvm-release
  shopt -s nullglob
  OUT_SOS=( target-llvm-release/release/lib*.so )
  shopt -u nullglob
  if [[ ${#OUT_SOS[@]} -ne 1 ]]; then
    echo "Expected exactly one cdylib under target-llvm-release/release/, found ${#OUT_SOS[@]}" >&2
    printf '  - %s\n' "${OUT_SOS[@]:-}" >&2
    exit 1
  fi
  cp "${OUT_SOS[0]}" ../benchmark.llvm.release.so
  cd -
)


(
  cd rust-benchmark-native
  cargo build --target-dir target-llvm-debug
  shopt -s nullglob
  OUT_SOS=( target-llvm-debug/debug/lib*.so )
  shopt -u nullglob
  if [[ ${#OUT_SOS[@]} -ne 1 ]]; then
    echo "Expected exactly one cdylib under target-llvm-debug/debug/, found ${#OUT_SOS[@]}" >&2
    printf '  - %s\n' "${OUT_SOS[@]:-}" >&2
    exit 1
  fi
  cp "${OUT_SOS[0]}" ../benchmark.llvm.debug.so
  cd -
)
