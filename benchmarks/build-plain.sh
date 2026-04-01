#!/usr/bin/env bash

# Build a "no-opt" (plain) variant of a benchmark using the benchmark's
# `Dockerfile`, but patching common optimization knobs to be disabled.
#
# The generated Wasm files are written next to the original ones, with
# `.plain.wasm` suffixes (e.g. `benchmark.plain.wasm`). Expected output files
# are shared with the base benchmark basename.
#
# Usage: ./build-plain.sh <path to benchmark directory>

set -euo pipefail

BENCHMARK_DIR=${1:-}
if [[ -z "${BENCHMARK_DIR}" || ! -d "${BENCHMARK_DIR}" ]]; then
    echo "Unknown benchmark directory; usage: ./build-plain.sh <path to benchmark directory>" >&2
    exit 1
fi

DOCKER=${DOCKER:-docker}
# BuildKit sometimes gets canceled in CI/editor environments; default to legacy
# builder unless the caller explicitly opts in.
export DOCKER_BUILDKIT=${DOCKER_BUILDKIT:-0}
BENCHMARK_NAME=$(readlink -f "${BENCHMARK_DIR}" | xargs basename)
IMAGE_NAME=sightglass-benchmark-plain-"${BENCHMARK_NAME}"

# From https://stackoverflow.com/a/246128:
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";
SIGHTGLASS_CARGO_TOML=$(dirname "${SCRIPT_DIR}")/Cargo.toml

# Helpful logging function.
print_header() {
    >&2 echo
    >&2 echo ===== "$@" =====
}

TMP_CTX=$(mktemp -d /tmp/sightglass-benchmark-plain-ctx-XXXXXX)
TMP_OUT=$(mktemp -d /tmp/sightglass-benchmark-plain-out-XXXXXX)

cleanup() {
    rm -rf "${TMP_CTX}" "${TMP_OUT}" || true
}
trap cleanup EXIT

print_header "Create build context (dereference symlinks)"
TMP_TAR=$(mktemp /tmp/sightglass-benchmark-plain-dir-XXXXXX.tar)
(set -x; cd "${BENCHMARK_DIR}" && tar --create --file "${TMP_TAR}" --dereference --verbose .)
(set -x; tar -xf "${TMP_TAR}" -C "${TMP_CTX}")

if [[ ! -f "${TMP_CTX}/Dockerfile" ]]; then
    echo "No Dockerfile found in ${BENCHMARK_DIR}; cannot build plain variant" >&2
    exit 1
fi

print_header "Patch build files for no-opt build"

# Patch the Dockerfile in a conservative, text-based way. We keep this intentionally
# simple and focused on the known patterns in this repo.
#
# - Flip C/C++ opt flags: -O3/-O2/-Os -> -O0
# - Rust: prefer debug profile (remove --release) and adjust copy path to /debug/
# - Emscripten: drop -DNDEBUG (often used to remove checks) and drop wasm stripping
perl -0777 -pe '
  s/(?:^|\s)(-O3|-O2|-Os)\b/ -O0/gm;
  s/\bcargo\s+build\s+--release\b/cargo build/gm;
  s@target/(wasm32-wasi|wasm32-wasip1)/release/@target/$1/debug/@gm;
  s/\s-DNDEBUG\b//gm;
' "${TMP_CTX}/Dockerfile" > "${TMP_CTX}/Dockerfile.plain"
mv "${TMP_CTX}/Dockerfile.plain" "${TMP_CTX}/Dockerfile"

# Some benchmarks run an internal build script that carries its own flags.
# Patch those too when present.
if [[ -f "${TMP_CTX}/build.sh" ]]; then
    perl -0777 -pe '
      s/(?:^|\s)(-O3|-O2|-Os)\b/ -O0/gm;
      s@^\s*/opt/wasi-sdk/bin/strip\s+(/benchmark/.*\.wasm)\s*$@# strip disabled for plain build: /opt/wasi-sdk/bin/strip $1@gm;
      s@^\s*strip\s+(/benchmark/.*\.wasm)\s*$@# strip disabled for plain build: strip $1@gm;
    ' "${TMP_CTX}/build.sh" > "${TMP_CTX}/build.sh.plain"
    mv "${TMP_CTX}/build.sh.plain" "${TMP_CTX}/build.sh"
    chmod +x "${TMP_CTX}/build.sh" || true
fi

print_header "Build benchmark image"
(set -x; "${DOCKER}" build --tag "${IMAGE_NAME}" "${TMP_CTX}")

print_header "Extract /benchmark outputs"
CONTAINER_ID=$("${DOCKER}" create "${IMAGE_NAME}")
(set -x; "${DOCKER}" cp "${CONTAINER_ID}":/benchmark/. "${TMP_OUT}")

print_header "Verify and install plain wasm artifacts"
for WASM in "${TMP_OUT}"/*.wasm; do
    (set -x; cargo run --manifest-path "${SIGHTGLASS_CARGO_TOML}" --quiet -- validate "${WASM}")

    BASE=$(basename "${WASM}")
    PLAIN_BASE="${BASE%.wasm}.plain.wasm"

    (set -x; mv "${WASM}" "${BENCHMARK_DIR}/${PLAIN_BASE}")

done

print_header "Clean up"
(set -x; rm "${TMP_TAR}")
(set -x; "${DOCKER}" rm "${CONTAINER_ID}" >/dev/null)
