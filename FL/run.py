import sys
import numpy as np
import tensorflow as tf
from fl.fl import FL

if __name__ == "__main__":
    device = '/GPU:0' if tf.config.list_physical_devices('GPU') else '/CPU:0'
    print(f"Device: {device}")

    if device == '/CPU:0':
        sys.exit()  

    fl = FL(scheduler="RR",
            n_seed=15,
            n_clients=150,
            n_rounds=100,
            model_type="CNN",           
            path_clients="/home/lenav40/Documentos/Datasets/mnist/ds", # mnist / fashion-mnist
            path_server="/home/lenav40/Documentos/Datasets/mnist/mnist
            )
    fl.run()

