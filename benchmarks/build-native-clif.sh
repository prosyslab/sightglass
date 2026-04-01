#!/usr/bin/env bash
#
# Build a single Rust Sightglass benchmark as a native shared library (benchmark.so)
# using rustc_codegen_cranelift's cargo wrapper (cargo-clif).
#
# Usage:
#   ./build-native-clif.sh --variant <v-*> --cargo-clif <path/to/cargo-clif> [--out <out.so>] <benchmark_dir>
#
set -euo pipefail
(set -x;)

usage() {
  cat >&2 <<'EOF'
Usage:
  build-native-clif.sh --variant <name> --cargo-clif <path> [--out <out.so>] <benchmark_dir>

Notes:
  - <benchmark_dir> must contain ./rust-benchmark and ./sightglass.native.patch
  - If <benchmark_dir>/setup.sh exists, it will be run before building.
EOF
}

VARIANT=""
CARGO_CLIF=""
OUT_SO=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --variant)
      VARIANT="${2:-}"; shift 2;;
    --cargo-clif)
      CARGO_CLIF="${2:-}"; shift 2;;
    --out)
      OUT_SO="${2:-}"; shift 2;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      break
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage
      exit 2
      ;;
    *)
      break
      ;;
  esac
done

BENCHMARK_DIR="${1:-}"
if [[ -z "$BENCHMARK_DIR" ]]; then
  usage
  exit 2
fi
if [[ ! -d "$BENCHMARK_DIR" ]]; then
  echo "Benchmark dir not found: $BENCHMARK_DIR" >&2
  exit 2
fi

if [[ -z "$VARIANT" ]]; then
  echo "Missing --variant" >&2
  exit 2
fi
if [[ -z "$CARGO_CLIF" ]]; then
  echo "Missing --cargo-clif" >&2
  exit 2
fi
if [[ ! -x "$CARGO_CLIF" ]]; then
  echo "cargo-clif not found or not executable: $CARGO_CLIF" >&2
  exit 2
fi

if [[ -z "$OUT_SO" ]]; then
  OUT_SO="$BENCHMARK_DIR/benchmark.so"
elif [[ "$OUT_SO" != /* ]]; then
  # Resolve relative output paths relative to the benchmark dir.
  OUT_SO="$BENCHMARK_DIR/$OUT_SO"
fi

if [[ ! -d "$BENCHMARK_DIR/rust-benchmark" ]]; then
  echo "Missing $BENCHMARK_DIR/rust-benchmark" >&2
  exit 2
fi
if [[ ! -f "$BENCHMARK_DIR/sightglass.native.patch" ]]; then
  echo "Missing $BENCHMARK_DIR/sightglass.native.patch" >&2
  exit 2
fi

# Some benchmarks require setup to fetch model/assets.
if [[ -f "$BENCHMARK_DIR/setup.sh" ]]; then
  (cd "$BENCHMARK_DIR" && bash ./setup.sh)
fi

# Keep variant builds isolated so multiple variants can coexist.
WORKDIR="$BENCHMARK_DIR/rust-benchmark-native-$VARIANT"
TARGET_DIR="target-clif-$VARIANT"

rm -rf "$WORKDIR"
cp -r "$BENCHMARK_DIR/rust-benchmark" "$WORKDIR/"
cp "$BENCHMARK_DIR/sightglass.native.patch" "$WORKDIR/"
(cd "$WORKDIR"; patch -Np1 -i ./sightglass.native.patch)

(
  cd "$WORKDIR"
  "$CARGO_CLIF" build --release --target-dir "$TARGET_DIR"
  shopt -s nullglob
  OUT_SOS=( "$TARGET_DIR"/release/lib*.so )
  shopt -u nullglob
  if [[ ${#OUT_SOS[@]} -ne 1 ]]; then
    echo "Expected exactly one cdylib under $WORKDIR/$TARGET_DIR/release/, found ${#OUT_SOS[@]}" >&2
    printf '  - %s\n' "${OUT_SOS[@]:-}" >&2
    exit 1
  fi
  cp "${OUT_SOS[0]}" "$OUT_SO"
)


