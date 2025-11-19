#! /usr/bin/env fish

set RUST_VERSION "1.88.0"
set WASMTIME_REPO "git@github.com:prosyslab/wasmtime.git"

rustup default $RUST_VERSION

pushd engines
pushd wasmtime
rustc build.rs # this generates ./build

if test (count $argv) -gt 0
  set VARIANTS $argv
else
  set VARIANTS base opts llvm-opts hydra
end

for variant in $VARIANTS 
	echo "############## $variant #############"
	mkdir -p bench-$variant
	REPOSITORY=$WASMTIME_REPO REVISION=bench-$variant ./build bench-$variant
end

