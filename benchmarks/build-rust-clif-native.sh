#!/usr/bin/env bash
#
# Build all benchmarks listed in rust-clif.suite as native shared libraries:
#   - LLVM baseline:   benchmark.llvm.so
#   - CLIF per variant: benchmark.<variant>.so
#
# This script expects rustc_codegen_cranelift variants to already be built, i.e.:
#   compilers/rustc_codegen_cranelift/target-<variant>/dist/cargo-clif
#
set -euo pipefail
(set -x;)

usage() {
  cat >&2 <<'EOF'
Usage:
  ./build-rust-clif-native.sh [--variants v-base,v-human,...] [--rustc-clif-dir <dir>] [--rebuild]

Outputs per benchmark directory:
  - benchmark.llvm.so
  - benchmark.<variant>.so  (for each variant)
EOF
}

REBUILD=0
VARIANTS_CSV="v-base,v-human,v-transopt-only,v-hydra,v-main"
RUSTC_CLIF_DIR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --variants)
      VARIANTS_CSV="${2:-}"; shift 2;;
    --rustc-clif-dir)
      RUSTC_CLIF_DIR="${2:-}"; shift 2;;
    --rebuild)
      REBUILD=1; shift 1;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

# Assumes this script is located at the base of the benchmark directory
BENCHMARKS_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
SUITE_FILE="$BENCHMARKS_DIR/rust-clif.suite"

if [[ -z "$RUSTC_CLIF_DIR" ]]; then
  REPO_ROOT=$(cd "$BENCHMARKS_DIR/../../.." && pwd)
  RUSTC_CLIF_DIR="$REPO_ROOT/compilers/rustc_codegen_cranelift"
fi

if [[ ! -f "$SUITE_FILE" ]]; then
  echo "Suite file not found: $SUITE_FILE" >&2
  exit 1
fi

SIGHTGLASS_BASE=$(dirname "$BENCHMARKS_DIR")
ENGINE_SO="$SIGHTGLASS_BASE/engines/native/libengine.so"

print_header() {
  >&2 echo
  >&2 echo "===== $* ====="
}

print_header "Ensure native engine is built"
if [[ ! -f "$ENGINE_SO" || $REBUILD -eq 1 ]]; then
  (cd "$SIGHTGLASS_BASE/engines/native/libengine/" && cargo build --release)
  (cd "$SIGHTGLASS_BASE/engines/native/libengine/" && cp target/release/libnative_bench_api.so ../libengine.so)
fi
if [[ ! -f "$ENGINE_SO" ]]; then
  echo "Missing native engine: $ENGINE_SO" >&2
  exit 1
fi

BUILD_CLIF="$BENCHMARKS_DIR/build-native-clif.sh"
if [[ ! -f "$BUILD_CLIF" ]]; then
  echo "Missing helper script: $BUILD_CLIF" >&2
  exit 1
fi

IFS=',' read -r -a VARIANTS <<<"$VARIANTS_CSV"
if [[ ${#VARIANTS[@]} -eq 0 ]]; then
  echo "No variants specified (empty --variants)" >&2
  exit 2
fi

# Read benchmarks from the suite file, skipping empty lines
while IFS= read -r benchmark || [[ -n "$benchmark" ]]; do
  [[ -z "$benchmark" ]] && continue

  bench_dir="$BENCHMARKS_DIR/$benchmark"
  if [[ ! -d "$bench_dir" ]]; then
    echo "Warning: benchmark directory not found: $bench_dir" >&2
    continue
  fi
  if [[ ! -f "$bench_dir/build-native.sh" ]]; then
    echo "Warning: missing $bench_dir/build-native.sh" >&2
    continue
  fi

  print_header "LLVM native build: $benchmark -> benchmark.llvm.release.release.so"
  llvm_out="$bench_dir/benchmark.llvm.release.so"
  if [[ -f "$llvm_out" && $REBUILD -eq 0 ]]; then
    echo "Already exists: $llvm_out (skipping)"
  else
    (cd "$bench_dir" && bash ./build-native.sh)
    if [[ ! -f "$bench_dir/benchmark.llvm.release.so" ]]; then
      echo "Missing $bench_dir/benchmark.llvm.release.so after LLVM build for $benchmark" >&2
      exit 1
    fi
  fi

  print_header "LLVM native build: $benchmark -> benchmark.llvm.debug.so"
  llvm_out="$bench_dir/benchmark.llvm.debug.so"
  if [[ -f "$llvm_out" && $REBUILD -eq 0 ]]; then
    echo "Already exists: $llvm_out (skipping)"
  else
    (cd "$bench_dir" && bash ./build-native.sh)
    if [[ ! -f "$bench_dir/benchmark.llvm.debug.so" ]]; then
      echo "Missing $bench_dir/benchmark.llvm.debug.so after LLVM build for $benchmark" >&2
      exit 1
    fi
  fi

  for v in "${VARIANTS[@]}"; do
    cargo_clif="$RUSTC_CLIF_DIR/target-$v/dist/cargo-clif"
    out="$bench_dir/benchmark.$v.so"
    print_header "CLIF native build: $benchmark [$v] -> $(basename "$out")"
    if [[ -f "$out" && $REBUILD -eq 0 ]]; then
      echo "Already exists: $out (skipping)"
      continue
    fi
    if [[ ! -x "$cargo_clif" ]]; then
      echo "Missing cargo-clif for variant $v: $cargo_clif" >&2
      echo "Build it first (see lib/variants.py RustcClifVariant.build)." >&2
      exit 1
    fi
    bash "$BUILD_CLIF" --variant "$v" --cargo-clif "$cargo_clif" --out "$out" "$bench_dir"
  done
done < "$SUITE_FILE"

print_header "Done"
