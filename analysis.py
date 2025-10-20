import argparse
from pathlib import Path

import pandas as pd
import numpy as np


parser = argparse.ArgumentParser()
parser.add_argument("benchmark_result", type=Path)

args = parser.parse_args()

df = pd.read_csv(args.benchmark_result, header=0)
df["wasm"] = df["wasm"].str.extract(r'benchmarks/(.*?)/benchmark.wasm')
print(df.columns)

stats_df = df.groupby(["wasm", "phase"])["count"].agg(["min", "mean", "max"]).reset_index()

print(stats_df)
stats_df.to_csv("benchmark.summary.csv", index=False)

