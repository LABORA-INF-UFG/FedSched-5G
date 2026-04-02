import os

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
os.environ.setdefault("TF_GPU_ALLOCATOR", "cuda_malloc_async")  # <<< troca o allocator

import argparse
import sys
import numpy as np
import tensorflow as tf
from fl.fl import FL

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument("--scheduler", type=str, default="RR")
    parser.add_argument("--n_seed", type=int, default=15)
    parser.add_argument("--n_clients", type=int, default=150)
    parser.add_argument("--n_rounds", type=int, default=100)
    parser.add_argument("--model_type", type=str, default="CNN")
    parser.add_argument("--path_clients", type=str, required=True)
    parser.add_argument("--path_server", type=str, required=True)
    parser.add_argument("--path_fl_results", type=str, required=True)

    args = parser.parse_args()

    device = '/GPU:0' if tf.config.list_physical_devices('GPU') else '/CPU:0'
    print(f"Device: {device}")

    if device == '/CPU:0':
        sys.exit()  # pass

    fl = FL(scheduler=args.scheduler,
            n_seed=args.n_seed,
            n_clients=args.n_clients,
            n_rounds=args.n_rounds,
            model_type=args.model_type,
            path_clients=args.path_clients,
            path_server=args.path_server,
            path_fl_results=args.path_fl_results
            )
    fl.run()
