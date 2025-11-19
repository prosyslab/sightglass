#! /usr/bin/env fish

# ======================== CONFIGURATIONS =========================
argparse 'i/iter=' 'p/phase=+' 'v/variant=+' 'h/help' -- $argv
or return

if set -q _flag_help
    echo "Usage: ./measure.fish [-i ITER] [-p PHASE...] [-v VARIANT...]"
    echo "Defaults:"
    echo "  ITER: 10"
    echo "  PHASE: compilation execution"
    echo "  VARIANTS: base opts llvm-opts hydra"
    return 0
end

set -l ITER 10
set -ql _flag_iter[1]; and set ITER $_flag_iter[-1]

set -l PHASE compilation execution
set -ql _flag_phase[1]; and set PHASE $_flag_phase

set -l VARIANTS base opts llvm-opts hydra
set -ql _flag_variant[1]; and set VARIANTS $_flag_variant

set -l OUTPUT "bench.csv"
set -l OUTFMT csv

echo "----------------------------------------"
echo "CONFIG | Iter: $ITER"
echo "       | Phase: $PHASE"
echo "       | Variants: $VARIANTS"
echo "----------------------------------------"

# ======================== CONFIGURATIONS =========================

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

