#! /usr/bin/env fish

# ======================== CONFIGURATIONS =========================
argparse 'n/name=' 'i/iter=' 'p/phase=+' 'v/variant=+' 'h/help' -- $argv
or return

if set -q _flag_help
    echo "Usage: ./measure.fish [-i ITER] [-p PHASE...] [-v VARIANT...]"
    echo "Defaults:"
    echo "  ITER: 10"
    echo "  PHASE: compilation execution"
    echo "  VARIANTS: base opts llvm-opts hydra"
    return 0
end

set -l NAME "bench"
set -ql _flag_name[1]; and set NAME $_flag_name[-1]

set -l ITER 10
set -ql _flag_iter[1]; and set ITER $_flag_iter[-1]

set -l PHASE compilation execution
set -ql _flag_phase[1]; and set PHASE $_flag_phase

set -l VARIANTS base opts llvm-opts hydra
set -ql _flag_variant[1]; and set VARIANTS $_flag_variant

set -l OUTPUT "$NAME.csv"
set -l OUTFMT csv

echo "----------------------------------------"
echo "NAME: $NAME"
echo "Iter: $ITER"
echo "Phase: $PHASE"
echo "Variants: $VARIANTS"
echo "----------------------------------------"

# ======================== CONFIGURATIONS =========================

echo "Program: $argv"
if test (count $argv) -gt 1
  echo "Only default suite and a single program can be measured at once"
  exit 1
end

rm $OUTPUT
echo "engine,wasm,phase,count" > $OUTPUT
rm *.log *.stderr

for variant in $VARIANTS
	set -l engine "engines/wasmtime/bench-$variant/libengine.so"
	set -l output "benchmark-$variant.$NAME.result.csv"
	echo "$engine => $output"

  set -l LOG $variant.$NAME.stderr
  echo "LOG: $LOG"
  if test -e $LOG
    rm $LOG
  end
  touch $LOG
	cargo run -- benchmark --engine $engine \
		--iterations-per-process $ITER \
		--raw --output-format $OUTFMT -- $argv > $output 2>> $LOG

	if test $status -ne 0 
		echo "$variant benchmark test failed!"
  end

	python3 clean.py $output >> $OUTPUT
end
wc -l $OUTPUT
./summary.py $OUTPUT


