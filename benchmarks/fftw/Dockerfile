FROM ubuntu:22.04 AS builder

ARG WASI_SDK_VERSION=20
ARG FFTW_VERSION=3.3.11
ARG FFTW_MD5=40ec8d0447d03b8f01f8c90aa77bd16f

RUN apt-get update && apt-get install -y \
    ca-certificates \
    cmake \
    make \
    ninja-build \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /
RUN wget "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz" \
    && tar xf "wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz"

WORKDIR /usr/src
RUN wget "https://www.fftw.org/fftw-${FFTW_VERSION}.tar.gz" \
    && echo "${FFTW_MD5}  fftw-${FFTW_VERSION}.tar.gz" | md5sum --check \
    && tar xf "fftw-${FFTW_VERSION}.tar.gz"

WORKDIR /usr/src/fftw-${FFTW_VERSION}
RUN ./configure \
    --host=wasm32-wasi \
    --build=x86_64-pc-linux-gnu \
    --prefix=/usr/local/fftw \
    --disable-shared \
    --enable-static \
    --disable-fortran \
    --disable-threads \
    CC="/wasi-sdk-${WASI_SDK_VERSION}.0/bin/clang" \
    AR="/wasi-sdk-${WASI_SDK_VERSION}.0/bin/llvm-ar" \
    RANLIB="/wasi-sdk-${WASI_SDK_VERSION}.0/bin/llvm-ranlib" \
    CFLAGS="--target=wasm32-wasi --sysroot=/wasi-sdk-${WASI_SDK_VERSION}.0/share/wasi-sysroot -O3 -DNDEBUG"
RUN make -j"$(nproc)"
RUN make install

WORKDIR /usr/src/benchmark-src
COPY CMakeLists.txt .
COPY benchmark.c .
COPY default.input .
COPY sightglass.h .

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/wasi-sdk-${WASI_SDK_VERSION}.0/share/cmake/wasi-sdk.cmake \
    -DWASI_SDK_PREFIX=/wasi-sdk-${WASI_SDK_VERSION}.0 \
    -DCMAKE_SYSROOT=/wasi-sdk-${WASI_SDK_VERSION}.0/share/wasi-sysroot \
    -DCMAKE_C_COMPILER=/wasi-sdk-${WASI_SDK_VERSION}.0/bin/clang \
    -DFFTW_PREFIX=/usr/local/fftw
RUN cmake --build build --target benchmark

RUN mkdir /benchmark
RUN cp build/benchmark.wasm /benchmark/benchmark.wasm
