
import argparse
from pathlib import Path

import pandas as pd
import numpy as np


parser = argparse.ArgumentParser()
parser.add_argument("benchmark_result", type=Path)
parser.add_argument("--header", action="store_true")
args = parser.parse_args()

input_file: Path = args.benchmark_result

df = pd.read_csv(input_file, header=0).drop(columns=["arch", "process", "iteration", "event"])
# benchmarks/shootout/shootout-memmove.wasm
# benchmarks/pulldown-cmark/benchmark.wasm
pat = r'benchmarks/(.*?)/benchmark\.wasm|benchmarks/shootout/(.*?)\.wasm'
extracted = df["wasm"].str.extract(pat)
df["wasm"] = extracted[0].fillna(extracted[1])
# df["wasm"] = df["wasm"].str.extract(pat)
# df["wasm"] = df["wasm"].str.extract(r'benchmarks/(.*?)/benchmark.wasm')

is_baseline = df["engine"].str.contains("bench-base")
is_opts = df["engine"].str.contains("bench-opts")
is_llvm_opts = df["engine"].str.contains("bench-llvm-opts")
conditions = [ is_baseline, is_opts, is_llvm_opts ]
choices = [ "BASE", "OPTS", "OPTS-LLVM" ]
df["engine"] = np.select(conditions, choices, default="HYDRA")

print(df.to_csv(index=False, header=args.header), end="")

