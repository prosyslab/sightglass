#! /usr/bin/env fish

set RUST_VERSION "1.88.0"
set WASMTIME_REPO "git@github.com:prosyslab/wasmtime.git"

rustup default $RUST_VERSION

pushd engines
pushd wasmtime
rustc build.rs # this generates ./build

mkdir bench-opts
mkdir bench-base
mkdir bench-hydra

# RQ2
REPOSITORY=$WASMTIME_REPO REVISION=bench-base-no-opts ./build bench-base
REPOSITORY=$WASMTIME_REPO REVISION=bench-opts ./build bench-opts

# RQ3
REPOSITORY=$WASMTIME_REPO REVISION=bench-llvm-opts ./build bench-llvm-opts
REPOSITORY=$WASMTIME_REPO REVISION=bench-hydra-opts ./build bench-hydra

