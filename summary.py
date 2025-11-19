#! /usr/bin/env python3

import argparse
import pandas as pd

ENGINES = ["BASE", "OPTS", "OPTS-LLVM", "HYDRA"]

def make_table(df: pd.DataFrame, phase: str):
    result = []
    for engine in ENGINES:
        performance = df[(df['engine'] == engine) & (df['phase'] == phase)].groupby('wasm')['count'].mean().rename(engine)
        result.append(performance)
    return pd.concat(result, axis=1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("input_csv")

    args = parser.parse_args()

    df = pd.read_csv(args.input_csv, dtype={"count": int})
    pd.set_option('display.float_format', '{:,.2f}'.format)

    print("RQ2")
    print("EXECUTION")
    print(make_table(df, "Execution").to_csv())
    print("Compilation")
    print(make_table(df, "Compilation").to_csv())

