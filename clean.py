
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
df["wasm"] = df["wasm"].str.extract(r'benchmarks/(.*?)/benchmark.wasm')
is_baseline = df["engine"].str.contains("base")
is_opts = df["engine"].str.contains("opts")
conditions = [ is_baseline, is_opts ]
choices = [ "BASE", "OPTS" ]
df["engine"] = np.select(conditions, choices, default="HYDRA")
print(df.to_csv(index=False, header=args.header), end="")

