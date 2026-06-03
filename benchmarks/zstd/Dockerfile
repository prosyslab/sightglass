FROM ubuntu:22.04 AS builder

ARG WASI_SDK_VERSION=20
ARG ZSTD_VERSION=v1.5.7
ARG ZSTD_COMMIT=f8745da6ff1ad1e7bab384bd1f9d742439278e99

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
RUN git clone --branch "${ZSTD_VERSION}" --depth 1 https://github.com/facebook/zstd.git \
    && cd zstd \
    && test "$(git rev-parse HEAD)" = "${ZSTD_COMMIT}"

WORKDIR /usr/src/benchmark-src
COPY CMakeLists.txt .
COPY benchmark.c .
COPY sightglass.h .

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/wasi-sdk-${WASI_SDK_VERSION}.0/share/cmake/wasi-sdk.cmake \
    -DWASI_SDK_PREFIX=/wasi-sdk-${WASI_SDK_VERSION}.0 \
    -DCMAKE_SYSROOT=/wasi-sdk-${WASI_SDK_VERSION}.0/share/wasi-sysroot \
    -DCMAKE_C_COMPILER=/wasi-sdk-${WASI_SDK_VERSION}.0/bin/clang \
    -DZSTD_ROOT=/usr/src/zstd
RUN cmake --build build --target benchmark

RUN mkdir /benchmark
RUN cp build/benchmark.wasm /benchmark/benchmark.wasm
