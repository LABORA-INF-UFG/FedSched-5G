#!/usr/bin/env python3

import argparse
import os
import random
import warnings
from collections import Counter

import numpy as np
import pandas as pd
import tensorflow as tf
from PIL import Image
import ot

warnings.filterwarnings("ignore")

SEED = 1

def load_dataset(dataset_name):
    if dataset_name == "mnist":
        dataset = tf.keras.datasets.mnist
    elif dataset_name == "fashion_mnist":
        dataset = tf.keras.datasets.fashion_mnist
    else:
        raise ValueError("dataset_name must be 'mnist' or 'fashion_mnist'")

    (train_images, train_labels), (test_images, test_labels) = dataset.load_data()
    return train_images, train_labels, test_images, test_labels


def rotacionar(df, py_rng):
    array_final = df.drop(columns=["label"]).to_numpy()

    for j, _ in enumerate(array_final):
        n_rand = py_rng.randint(-15, 15)

        imagem_original = array_final[j].reshape(28, 28).copy()
        imagem_pil = Image.fromarray(imagem_original)
        imagem_rotacionada = np.array(imagem_pil.rotate(n_rand))
        array_final[j] = imagem_rotacionada.flatten()

    df_final = pd.DataFrame(array_final)
    df_final.reset_index(drop=True, inplace=True)
    df_final["label"] = df["label"].values
    return df_final


def calcular_histograma(v_labels):
    suporte = np.arange(10)
    contagens = np.array([np.sum(v_labels == r) for r in suporte])
    total = contagens.sum()
    if total == 0:
        return np.zeros_like(contagens, dtype=float)
    return contagens / total


def definir_matriz_custo():
    suporte = np.arange(10)
    n = len(suporte)
    C = np.zeros((n, n))
    for i in range(n):
        for j in range(n):
            C[i, j] = abs(suporte[i] - suporte[j])
    return C


def calcular_emd_penalizado(v_labels, gamma=5.0):
    P = calcular_histograma(v_labels)
    n = len(P)
    U = np.ones(n) / n
    C = definir_matriz_custo()

    emd_value = ot.emd2(P, U, C)

    missing_bins = np.sum(P == 0)
    penalty_factor = 1 + gamma * (missing_bins / n)

    return emd_value * penalty_factor


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset_name", type=str, required=True, choices=["mnist", "fashion_mnist"])
    parser.add_argument("--num_clients", type=int, required=True)
    parser.add_argument("--clients_by_alpha", type=int, required=True)

    args = parser.parse_args()

    dataset_name = args.dataset_name
    num_clients = args.num_clients
    clients_by_alpha = args.clients_by_alpha
    
    print(f"dataset_name: {dataset_name}")

    np.random.seed(SEED)
    random.seed(SEED)
    tf.random.set_seed(SEED)

    rng_meta = np.random.RandomState(SEED)
    rng_partition = np.random.RandomState(SEED)
    rng_sample = np.random.RandomState(SEED)

    py_rng_single = random.Random(SEED)
    py_rng_dirichlet = random.Random(SEED)

    train_images, train_labels, test_images, test_labels = load_dataset(dataset_name)

    num_train_samples = train_images.shape[0]
    num_test_samples = test_images.shape[0]
    num_features = train_images.shape[1] * train_images.shape[2]

    train_images_reshaped = train_images.reshape((num_train_samples, num_features))
    test_images_reshaped = test_images.reshape((num_test_samples, num_features))

    df_train = pd.DataFrame(train_images_reshaped)
    df_test = pd.DataFrame(test_images_reshaped)

    df_train["label"] = train_labels
    df_test["label"] = test_labels

    base_dir = f"./{dataset_name}"
    one_label_dir = f"{base_dir}/one-label"
    full_dir = f"{base_dir}/{dataset_name}"
    output_dir = f"{base_dir}/ds"

    os.makedirs(one_label_dir, exist_ok=True)
    os.makedirs(full_dir, exist_ok=True)
    os.makedirs(output_dir, exist_ok=True)

    df_train_all = pd.DataFrame()
    df_test_all = pd.DataFrame()

    for i in range(10):
        df_train_aux = df_train[df_train["label"] == i]
        df_test_aux = df_test[df_test["label"] == i]

        df_train_all = pd.concat([df_train_all, df_train_aux])
        df_test_all = pd.concat([df_test_all, df_test_aux])

        df_train_aux = df_train_aux.reset_index(drop=True)
        df_test_aux = df_test_aux.reset_index(drop=True)

        df_train_aux.to_pickle(f"{one_label_dir}/{i+1}_train.pickle")
        df_test_aux.to_pickle(f"{one_label_dir}/{i+1}_test.pickle")

    df_train_all = df_train_all.reset_index(drop=True)
    df_test_all = df_test_all.reset_index(drop=True)

    df_train_all.to_pickle(f"{full_dir}/train.pickle")
    df_test_all.to_pickle(f"{full_dir}/test.pickle")

    # estrutura global
    low_values = rng_meta.uniform(200, 1000, size=int(num_clients * 0.8))
    high_values = rng_meta.uniform(200, 1000, size=int(num_clients * 0.2))
    size_train = np.concatenate((low_values, high_values))
    rng_meta.shuffle(size_train)

    ind_cids = rng_meta.permutation(np.arange(1, num_clients + 1))
    ind_grupo = [-1] * num_clients

    # único rótulo
    for i in range(clients_by_alpha):
        cid = ind_cids[i]
        ind_grupo[cid - 1] = 1

        tam_data = int(size_train[i])
        rotulo_unico = i % 10

        df_train_label = pd.read_pickle(f"{one_label_dir}/{rotulo_unico+1}_train.pickle")

        df_unico_train = df_train_label.sample(n=tam_data, replace=True, random_state=rng_sample)
        df_unico_test = df_train_label.sample(n=int(tam_data * 0.25), replace=True, random_state=rng_sample)

        final_train = rotacionar(df_unico_train.copy(), py_rng_single)
        final_test = rotacionar(df_unico_test.copy(), py_rng_single)

        final_train.to_pickle(f"{output_dir}/{cid}_train.pickle")
        final_test.to_pickle(f"{output_dir}/{cid}_test.pickle")

    # dirichlet
    num_classes = 10
    alphas = [0.1, 0.5, 1, 10]

    inicio_global = clients_by_alpha

    for k, alpha_value in enumerate(alphas):
        inicio = inicio_global + k * clients_by_alpha
        fim = inicio + clients_by_alpha

        alpha_vec = [alpha_value] * num_classes
        dirichlet_distribution = rng_partition.dirichlet(alpha_vec, size=clients_by_alpha)

        for local_idx, i in enumerate(range(inicio, fim)):
            cid = ind_cids[i]
            ind_grupo[cid - 1] = 2 + k

            tam_data = int(size_train[i])
            dist = dirichlet_distribution[local_idx]

            part_train = rng_partition.multinomial(tam_data, dist)
            part_test = rng_partition.multinomial(int(tam_data * 0.25), dist)

            df_base_train = pd.DataFrame()
            df_base_test = pd.DataFrame()

            for j in range(num_classes):
                if part_train[j] > 0:
                    df_train_label = pd.read_pickle(f"{one_label_dir}/{j+1}_train.pickle")
                    df_base_train = pd.concat(
                        [df_base_train,
                         df_train_label.sample(n=part_train[j], replace=True, random_state=rng_sample)],
                        ignore_index=True
                    )

                if part_test[j] > 0:
                    df_test_label = pd.read_pickle(f"{one_label_dir}/{j+1}_test.pickle")
                    df_base_test = pd.concat(
                        [df_base_test,
                         df_test_label.sample(n=part_test[j], replace=True, random_state=rng_sample)],
                        ignore_index=True
                    )

            if not df_base_train.empty:
                df_base_train = df_base_train.sample(frac=1, random_state=SEED).reset_index(drop=True)

            if not df_base_test.empty:
                df_base_test = df_base_test.sample(frac=1, random_state=SEED).reset_index(drop=True)

            final_train = rotacionar(df_base_train.copy(), py_rng_dirichlet)
            final_test = rotacionar(df_base_test.copy(), py_rng_dirichlet)

            final_train.to_pickle(f"{output_dir}/{cid}_train.pickle")
            final_test.to_pickle(f"{output_dir}/{cid}_test.pickle")

    # EMD
    emd_values = []
    for cid in range(1, num_clients + 1):
        df_client = pd.read_pickle(f"{output_dir}/{cid}_train.pickle")
        emd_values.append(calcular_emd_penalizado(df_client["label"].to_numpy()))

    with open(f"{output_dir}/info.txt", "w") as f:
        f.write(f"ind_cids:\n{ind_cids}\n\n")
        f.write(f"ind_grupo:\n{ind_grupo}\n\n")
        f.write(f"Counter(ind_grupo):\n{Counter(ind_grupo)}\n\n")
        f.write(f"EMD:\n{emd_values}\n")


if __name__ == "__main__":
    main()