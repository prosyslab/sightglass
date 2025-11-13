#! /usr/bin/env fish

set RUST_VERSION "1.88.0"
set WASMTIME_REPO "git@github.com:prosyslab/wasmtime.git"

rustup default $RUST_VERSION

pushd engines
pushd wasmtime
rustc build.rs # this generates ./build

mkdir -p bench-base
mkdir -p bench-opts
mkdir -p bench-llvm-opts
mkdir -p bench-hydra

# RQ2
REPOSITORY=$WASMTIME_REPO REVISION=bench-base-no-opts ./build bench-base
REPOSITORY=$WASMTIME_REPO REVISION=bench-opts ./build bench-opts

# FOR FUN
REPOSITORY=$WASMTIME_REPO REVISION=bench-gcc-opts ./build bench-gcc-opts
REPOSITORY=$WASMTIME_REPO REVISION=bench-go-opts ./build bench-go-opts

# RQ3
REPOSITORY=$WASMTIME_REPO REVISION=bench-llvm-opts ./build bench-llvm-opts
REPOSITORY=$WASMTIME_REPO REVISION=bench-hydra-opts ./build bench-hydra

