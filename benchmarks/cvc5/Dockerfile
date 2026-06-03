FROM emscripten/emsdk:3.1.70

RUN apt-get update && apt-get install -y \
    bison \
    flex \
    git \
    m4 \
    make \
    perl \
    python3 \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src
RUN git clone --branch cvc5-1.3.4 --depth 1 https://github.com/cvc5/cvc5.git

WORKDIR /usr/src/cvc5
RUN ./configure.sh production \
    --static \
    --static-binary \
    --auto-download \
    --no-editline \
    --no-java-bindings \
    --no-python-bindings \
    --no-unit-testing \
    --wasm=WASM \
    --prefix=/usr/local/cvc5 \
    --name=build

RUN cmake --build build --target install -j"$(nproc)"
RUN mkdir -p /usr/local/cvc5/lib/cmake/cvc5 \
    && cp build/cmake/cvc5Config*.cmake /usr/local/cvc5/lib/cmake/cvc5/ \
    && find build -name 'cvc5Targets*.cmake' -exec cp {} /usr/local/cvc5/lib/cmake/cvc5/ \; \
    && cp build/deps/lib/*.a /usr/local/cvc5/lib/ \
    && ls -l /usr/local/cvc5/lib/cmake/cvc5/cvc5Targets*.cmake

WORKDIR /usr/src/benchmark-src
ADD . .
RUN emcmake cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_MAP_IMPORTED_CONFIG_RELEASE=Production \
    -DCVC5_DEP_LIB_DIR=/usr/local/cvc5/lib \
    -Dcvc5_DIR=/usr/local/cvc5/lib/cmake/cvc5
RUN cmake --build build --target benchmark -j"$(nproc)"

RUN mkdir /benchmark
RUN cp build/benchmark.wasm /benchmark/benchmark.wasm
