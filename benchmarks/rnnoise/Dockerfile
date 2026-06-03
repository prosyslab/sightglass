FROM ubuntu:22.04 AS builder

ARG WASI_SDK_VERSION=20
ARG RNNOISE_VERSION=0.2
ARG RNNOISE_SHA256=90fce4b00b9ff24c08dbfe31b82ffd43bae383d85c5535676d28b0a2b11c0d37

RUN apt-get update && apt-get install -y \
    ca-certificates \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /
RUN wget "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz" \
    && tar xf "wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz"

RUN wget "https://github.com/xiph/rnnoise/releases/download/v${RNNOISE_VERSION}/rnnoise-${RNNOISE_VERSION}.tar.gz" \
    && echo "${RNNOISE_SHA256}  rnnoise-${RNNOISE_VERSION}.tar.gz" | sha256sum --check \
    && tar xf "rnnoise-${RNNOISE_VERSION}.tar.gz"

WORKDIR /usr/src/benchmark-src
COPY benchmark.c .
COPY sightglass.h .
COPY os_support.h .

ENV CC=/wasi-sdk-20.0/bin/clang
ENV CFLAGS=--sysroot=/wasi-sdk-20.0/share/wasi-sysroot
ENV RNNOISE_SRC=/rnnoise-0.2

RUN $CC $CFLAGS -O3 -DNDEBUG \
    -I. \
    -I$RNNOISE_SRC/include \
    -I$RNNOISE_SRC/src \
    benchmark.c \
    $RNNOISE_SRC/src/denoise.c \
    $RNNOISE_SRC/src/rnn.c \
    $RNNOISE_SRC/src/pitch.c \
    $RNNOISE_SRC/src/kiss_fft.c \
    $RNNOISE_SRC/src/celt_lpc.c \
    $RNNOISE_SRC/src/nnet.c \
    $RNNOISE_SRC/src/nnet_default.c \
    $RNNOISE_SRC/src/parse_lpcnet_weights.c \
    $RNNOISE_SRC/src/rnnoise_data.c \
    $RNNOISE_SRC/src/rnnoise_tables.c \
    -lm \
    -Wl,--initial-memory=134217728 \
    -Wl,-z,stack-size=8388608 \
    -o benchmark.wasm

RUN mkdir /benchmark
RUN cp benchmark.wasm /benchmark/benchmark.wasm
