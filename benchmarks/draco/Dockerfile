FROM ubuntu:22.04 AS builder

ARG WASI_SDK_VERSION=20
ARG DRACO_VERSION=1.5.7
ARG DRACO_COMMIT=8786740086a9f4d83f44aa83badfbea4dce7a1b5

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
RUN git clone --branch "${DRACO_VERSION}" --depth 1 https://github.com/google/draco.git \
    && cd draco \
    && test "$(git rev-parse HEAD)" = "${DRACO_COMMIT}"

WORKDIR /usr/src/benchmark-src
COPY CMakeLists.txt .
COPY benchmark.cpp .
COPY sightglass.h .

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/wasi-sdk-${WASI_SDK_VERSION}.0/share/cmake/wasi-sdk.cmake \
    -DWASI_SDK_PREFIX=/wasi-sdk-${WASI_SDK_VERSION}.0 \
    -DCMAKE_SYSROOT=/wasi-sdk-${WASI_SDK_VERSION}.0/share/wasi-sysroot \
    -DCMAKE_C_COMPILER=/wasi-sdk-${WASI_SDK_VERSION}.0/bin/clang \
    -DCMAKE_CXX_COMPILER=/wasi-sdk-${WASI_SDK_VERSION}.0/bin/clang++ \
    -DDRACO_ROOT=/usr/src/draco
RUN cmake --build build --target benchmark

RUN mkdir /benchmark
RUN cp build/benchmark.wasm /benchmark/benchmark.wasm
