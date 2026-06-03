FROM ubuntu:22.04 AS builder

ARG WASI_SDK_VERSION=20
ARG FLAC_VERSION=1.5.0
ARG FLAC_COMMIT=1507800de4b70e21be71f38caa0d9079d0bc6e45

RUN apt-get update && apt-get install -y \
    ca-certificates \
    cmake \
    git \
    ninja-build \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /
RUN wget "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz" \
    && tar xf "wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz"

WORKDIR /usr/src
RUN git clone --branch "${FLAC_VERSION}" --depth 1 https://github.com/xiph/flac.git \
    && cd flac \
    && test "$(git rev-parse HEAD)" = "${FLAC_COMMIT}"

WORKDIR /usr/src/benchmark-src
COPY CMakeLists.txt .
COPY benchmark.c .
COPY flac_wasi_compat.c .
COPY flac_wasi_compat.h .
COPY sightglass.h .

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/wasi-sdk-${WASI_SDK_VERSION}.0/share/cmake/wasi-sdk.cmake \
    -DWASI_SDK_PREFIX=/wasi-sdk-${WASI_SDK_VERSION}.0 \
    -DCMAKE_SYSROOT=/wasi-sdk-${WASI_SDK_VERSION}.0/share/wasi-sysroot \
    -DCMAKE_C_COMPILER=/wasi-sdk-${WASI_SDK_VERSION}.0/bin/clang \
    -DFLAC_ROOT=/usr/src/flac
RUN cmake --build build --target benchmark

RUN mkdir /benchmark
RUN cp build/benchmark.wasm /benchmark/benchmark.wasm
