#! /usr/bin/env fish

set -l ITER 20
set -l OUTFMT csv

set -l BASE_ENGINE "engines/wasmtime/bench-base/libengine.so"
set -l OPTS_ENGINE "engines/wasmtime/bench-opts/libengine.so"
set -l LLVM_OPTS_ENGINE "engines/wasmtime/bench-llvm-opts/libengine.so"
set -l HYDRA_ENGINE "engines/wasmtime/bench-hydra/libengine.so"

set -l BASE_OUTPUT "benchmark-base.result.csv"
set -l OPTS_OUTPUT "benchmark-opts.result.csv"
set -l LLVM_OPTS_OUTPUT "benchmark-llvm-opts.result.csv"
set -l HYDRA_OUTPUT "benchmark-hydra.result.csv"
set -l OUTPUT "bench.csv"

cargo run -- benchmark \
	--engine $BASE_ENGINE \
  --iterations-per-process $ITER \
	--raw --output-format $OUTFMT  \
	> $BASE_OUTPUT

cargo run -- benchmark \
	--engine $OPTS_ENGINE \
  --iterations-per-process $ITER \
	--raw --output-format $OUTFMT \
	> $OPTS_OUTPUT

cargo run -- benchmark \
	--engine $LLVM_OPTS_ENGINE \
  --iterations-per-process $ITER \
	--raw --output-format $OUTFMT \
	> $LLVM_OPTS_OUTPUT

cargo run -- benchmark \
	--engine $HYDRA_ENGINE \
  --iterations-per-process $ITER \
	--raw --output-format $OUTFMT \
	> $HYDRA_OUTPUT

python3 clean.py $BASE_OUTPUT --header > $OUTPUT
python3 clean.py $OPTS_OUTPUT >> $OUTPUT
python3 clean.py $LLVM_OPTS_OUTPUT >> $OUTPUT
python3 clean.py $HYDRA_OUTPUT >> $OUTPUT
wc -l $OUTPUT
./summary.py $OUTPUT


echo "STATISTICAL TESTS"

cargo run -- benchmark \
	--engine $BASE_ENGINE  --engine $OPTS_ENGINE \
	--iterations-per-process 20 -s 0.05 \
	--benchmark-phase execution > base-opts-statistics.execution

cargo run -- benchmark \
	--engine $BASE_ENGINE --engine $OPTS_ENGINE \
	--iterations-per-process 20 -s 0.05 \
	--benchmark-phase compilation > base-opts-statistics.compilation

cargo run -- benchmark \
	--engine $LLVM_OPTS_ENGINE --engine $HYDRA_ENGINE \
	--iterations-per-process 20 -s 0.05 \
	--benchmark-phase execution > opts-hydra-statistics.execution

cargo run -- benchmark \
	--engine $LLVM_OPTS_ENGINE --engine $HYDRA_ENGINE \
	--iterations-per-process 20 -s 0.05 \
	--benchmark-phase compilation > opts-hydra-statistics.compilation

rm *.log

