import sys
import numpy as np
import pandas as pd
import tensorflow as tf
from server.server import Server
from client.client import Client
from ml_model.ml_model import Model


class FL:
    def __init__(self, scheduler, n_seed, n_clients, n_rounds, model_type, path_clients, path_server):
        self.scheduler = scheduler
        self.n_seed = n_seed
        self.n_clients = n_clients
        self.n_rounds = n_rounds
        self.model_type = model_type
        self.path_clients = path_clients
        self.path_server = path_server


    def run(self):

        acc_list = []
        loss_list = []
        for i in range(self.n_seed):
            accuracy, loss = self.run_seed(i+1)
            acc_list.append(accuracy)
            loss_list.append(loss)

            print(f"\nFinal seed: {i+1}")
            print(f"accuracy: {accuracy}")
            print(f"loss: {loss}")

        print("-------------------")

        output_file = f"../fl-results/seed-avg/{self.scheduler}/acc_results-{self.model_type}.txt"
        with open(output_file, "w") as f:
            f.write(f"acc_mean:\n{np.mean(acc_list, axis=0).tolist()}\n\n")
            f.write(f"acc_std:\n{np.std(acc_list, axis=0).tolist()}\n\n")
            f.write(f"acc_min:\n{np.min(acc_list, axis=0).tolist()}\n\n")
            f.write(f"acc_max:\n{np.max(acc_list, axis=0).tolist()}\n\n")
            f.write(f"loss_mean:\n{np.mean(loss_list, axis=0).tolist()}\n\n")
            f.write(f"loss_std:\n{np.std(loss_list, axis=0).tolist()}\n\n")
            f.write(f"loss_min:\n{np.min(loss_list, axis=0).tolist()}\n\n")
            f.write(f"loss_max:\n{np.max(loss_list, axis=0).tolist()}")
        print(f"Resultados salvos em: {output_file}")


    def run_seed(self, seed):
        round_ids = self.success_ids(seed)   
        s = Server(self.n_rounds, self.n_clients, self.scheduler, self.model_type, self.path_clients, self.path_server)

        accuracy = []
        loss = []
        print(f"\n---> seed: {seed} | scheduler: {self.scheduler}")
        print(f"-----------------")

        for s.server_round in range(s.n_rounds):
            selected_clients = round_ids.get(s.server_round + 1)
            print(f"R: {s.server_round + 1} | selected_clients: {selected_clients}")

            if len(selected_clients) > 0:
                s.configure_fit(selected_clients)
                weight_list, sample_sizes, info = s.fit()
                s.aggregate_fit(weight_list, sample_sizes)

            evaluate_loss, evaluate_accuracy = s.centralized_evaluation()
            print(f"evaluate_accuracy: {evaluate_accuracy} | evaluate_loss: {evaluate_loss}")
            accuracy.append(evaluate_accuracy)
            loss.append(evaluate_loss)
            print(f"-----------------")
        return accuracy, loss


    def success_ids(self, seed):
        file_path = f"../fl-results/seed{seed}/{self.scheduler}/successIds.csv"
        df = pd.read_csv(file_path, sep=",")
        df = df.dropna(axis=1, how="all")

        round_dict = {}
        for r in range(1, self.n_rounds+1):
            if r in df.iloc[:, 0].values:  
                row = df[df.iloc[:, 0] == r].iloc[0]
                ids = []
                for val in row.iloc[1:].dropna().astype(str).values:
                    for v in val.replace(",", " ").replace(";", " ").split():
                        if v.strip().isdigit():
                            ids.append(int(v))
                round_dict[r] = ids
            else:                
                round_dict[r] = []
        return round_dict
