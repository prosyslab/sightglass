#! /usr/bin/env

set RUST_VERSION "1.88.0"
set WASMTIME_REPO "git@github.com:prosyslab/wasmtime.git"

rustup default $RUST_VERSION

pushd engines
pushd wasmtime
rustc build.rs # this generates ./build

mkdir bench-current
mkdir bench-base
REPOSITORY=$WASMTIME_REPO REVISION=main ./build bench-current
REPOSITORY=$WASMTIME_REPO REVISION=bench-base-no-opts ./build bench-base

