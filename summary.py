#! /usr/bin/env python3

import argparse
import pandas as pd

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("input_csv")

    args = parser.parse_args()

    df = pd.read_csv(args.input_csv, dtype={"count": int})

    base_perf = df[(df['engine'] == "Baseline") & (df['phase'] == "Execution")].groupby('wasm')['count'].mean()
    head_perf = df[(df['engine'] == "Head") & (df['phase'] == "Execution")].groupby('wasm')['count'].mean()
    gain = (base_perf - head_perf) / head_perf * 100 

    pd.set_option('display.float_format', '{:,.2f}'.format)
    print(pd.concat([base_perf, head_perf, gain], axis=1))


    # print(base_perf)
    # print(head_perf)
    # print(( base_perf - head_perf ) / head_perf * 100)

