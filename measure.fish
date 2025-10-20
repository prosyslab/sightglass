#! /usr/bin/env fish

cargo run -- benchmark \
	--engine engines/wasmtime/bench-base/libengine.so \
	--raw --output-format csv > benchmark-base.result.csv

cargo run -- benchmark \
	--engine engines/wasmtime/bench-current/libengine.so \
	--raw --output-format csv > benchmark-head.result.csv


python3 clean.py benchmark-base.result.csv --header > "bench.csv"
python3 clean.py benchmark-head.result.csv >> "bench.csv"
wc -l bench.csv

