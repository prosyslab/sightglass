#! /usr/bin/env fish

set -l ITER 10
set -l OUTFMT csv
set -l PHASE compilation execution
set -l VARIANTS base opts llvm-opts hydra # gcc-opts go-opts
set -l OUTPUT "bench.csv"

rm $OUTPUT
echo "engine,wasm,phase,count" > $OUTPUT
for variant in $VARIANTS
	set -l engine "engines/wasmtime/bench-$variant/libengine.so"
	set -l output "benchmark-$variant.result.csv"
	echo "$engine => $output"

	cargo run -- benchmark --engine $engine \
		--iterations-per-process $ITER \
		--raw --output-format $OUTFMT -- > $output 2> $variant.stderr

	if test $status -ne 0 
		echo "$variant benchmark test failed!"
  end

	python3 clean.py $output >> $OUTPUT
end
wc -l $OUTPUT
./summary.py $OUTPUT

rm *.log

