FROM ubuntu:22.04 AS builder

ARG WASI_SDK_VERSION=33
ARG WASI_SDK_SHA256=0ba8b5bfaeb2adf3f29bab5841d76cf5318ab8e1642ea195f88baba1abd47bce
ARG JPEG_TURBO_VERSION=3.1.4.1
ARG JPEG_TURBO_COMMIT=9217719d3a58633923b096af4c1d50d304768a64

RUN apt-get update && apt-get install -y \
    ca-certificates \
    cmake \
    git \
    ninja-build \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /
RUN wget "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/wasi-sdk-${WASI_SDK_VERSION}.0-x86_64-linux.tar.gz" \
    && echo "${WASI_SDK_SHA256}  wasi-sdk-${WASI_SDK_VERSION}.0-x86_64-linux.tar.gz" | sha256sum --check \
    && tar xf "wasi-sdk-${WASI_SDK_VERSION}.0-x86_64-linux.tar.gz" \
    && mv "wasi-sdk-${WASI_SDK_VERSION}.0-x86_64-linux" "wasi-sdk-${WASI_SDK_VERSION}.0"

WORKDIR /usr/src
RUN git clone --branch "${JPEG_TURBO_VERSION}" --depth 1 https://github.com/libjpeg-turbo/libjpeg-turbo.git \
    && cd libjpeg-turbo \
    && test "$(git rev-parse HEAD)" = "${JPEG_TURBO_COMMIT}"

WORKDIR /usr/src/libjpeg-turbo
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/wasi-sdk-${WASI_SDK_VERSION}.0/share/cmake/wasi-sdk.cmake \
    -DWASI_SDK_PREFIX=/wasi-sdk-${WASI_SDK_VERSION}.0 \
    -DCMAKE_SYSROOT=/wasi-sdk-${WASI_SDK_VERSION}.0/share/wasi-sysroot \
    -DCMAKE_C_COMPILER=/wasi-sdk-${WASI_SDK_VERSION}.0/bin/clang \
    -DCMAKE_C_FLAGS="-mllvm -wasm-enable-sjlj -mllvm -wasm-use-legacy-eh=false" \
    -DCMAKE_EXE_LINKER_FLAGS="-lsetjmp -Wl,-mllvm,-wasm-enable-sjlj,-mllvm,-wasm-use-legacy-eh=false" \
    -DCMAKE_INSTALL_PREFIX=/usr/local/jpeg-turbo \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DENABLE_SHARED=OFF \
    -DENABLE_STATIC=ON \
    -DWITH_SIMD=OFF \
    -DWITH_JAVA=OFF \
    -DWITH_TOOLS=OFF \
    -DWITH_TESTS=OFF \
    -DWITH_TURBOJPEG=ON
RUN cmake --build build --target install

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
    -DJPEG_TURBO_PREFIX=/usr/local/jpeg-turbo
RUN cmake --build build --target benchmark

RUN mkdir /benchmark
RUN cp build/benchmark.wasm /benchmark/benchmark.wasm
