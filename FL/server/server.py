import sys
import numpy as np
import pandas as pd
import tensorflow as tf
from client.client import Client
from ml_model.ml_model import Model


class Server:
    def __init__(self, n_rounds, n_clients, scheduler, model_type, path_clients, path_server):
        self.n_rounds = n_rounds
        self.n_clients = n_clients
        self.scheduler = scheduler
        self.model_type = model_type
        self.path_clients = path_clients
        self.path_server = path_server

        self.server_round = 0
        self.selected_clients = []

        if self.model_type == "CNN":
            self.model = Model.create_model_cnn()
        else:
            self.model = Model.create_model_mlp()

        print(f"count_params: {self.model.count_params()} params")
        self.w_global = self.model.get_weights()

        self.clients_model_list = []

        self.create_models()
        (self.x_test, self.y_test) = self.load_data()

        self.clients_emd = [
            0.17115384615384616, 0.422169811320755,   3.052985074626866,   0.6123595505617976,  5.855378486055777, 
            20.35,               1.6415019762845853,  0.2539906103286384,  0.11791044776119394, 0.7728971962616823, 
            0.5020202020202018,  0.6702586206896552,  1.8817669172932336,  0.20324574961360106, 24.75,             
            1.3062827225130889,  14.850000000000001,  5.142347600518807,   6.386100386100386,   17.050000000000004, 
            0.32848664688427304, 1.3328571428571427,  0.4969135802469133,  1.6724137931034486,  0.8009153318077804, 
            0.2048543689320389,  0.5950819672131148,  5.084684684684684,   11.021300448430493,  20.35, 
            1.4299999999999995,  20.35,               0.909040793825799,   14.850000000000001,  1.2674672489082945, 
            24.750000000000004,  9.821875,            0.31487236403995655, 0.3863636363636363,  11.25624256837099,
            14.850000000000001,  1.7337209302325576,  13.750000000000002,  0.39372384937238486, 17.050000000000004, 
            7.743491124260356,   0.6937500000000001,  0.1625,              5.053846153846154,   1.2543026706231455, 
            0.538961038961039,   14.850000000000001,  0.8541850220264315,  6.470661157024793,   7.027846674182637, 
            1.6249422632794457,  13.750000000000002,  0.28991769547325086, 12.28888888888889,   7.012048192771085, 
            0.2088676671214189,  0.17504690431519693, 1.3199066874027994,  0.1910256410256409,  0.5603709949409782, 
            3.0259740259740218,  17.050000000000004,  0.7511210762331838,  0.18934850051706306, 4.392307692307693, 
            0.6929203539823008,  0.2635359116022099,  0.3319427890345651,  20.35,               1.8427038626609429,
            17.050000000000004,  0.4524945770065074,  17.050000000000004,  13.750000000000002,  0.5887096774193545,
            1.6418604651162791,  0.35937072503419976, 1.1834437086092713,  1.5396440129449835,  20.35, 
            11.716872427983542,  0.10189753320683108, 1.4302063789868664,  0.5287769784172662,  5.280431432973806, 
            24.75,               6.0260303687,        14.850000000000001,  0.8338607594936711,  24.75,               
            12.104887218045114,  0.2099526066350711,  1.2526690391459074,  20.35,               13.750000000000002,  
            3.2322580645161283,  0.41788079470198675, 17.050000000000004,  6.215130023640661,   1.7742537313432838,  
            0.8645569620253164,  0.5702127659574471,  7.8095846645367395,  7.021889400921658,   1.6123595505617976,  
            2.6212499999999994,  1.1061538461538463,  7.2067235859124885,  1.3495106035889064,  3.080229226361032,   
            0.3616352201257862,  1.7595818815331001,  7.138888888888888,   17.149038461538463,  7.464227642276423,   
            24.750000000000004,  1.6779069767441857,  9.748706896551727,   3.7897590361445785,  1.7856600910470406,  
            13.750000000000002,  5.415454545454544,   3.8557851239669425,  11.78679245283019,   0.18683473389355731, 
            0.9489905787348585,  13.750000000000002,  4.526430976430976,   0.2455284552845528,  0.16269430051813472, 
            3.370625662778367,   0.537046307884856,   0.26267605633802804, 1.8991130820399114,  15.359098228663447,  
            0.16869220607661808, 14.850000000000001,  0.9520179372197308,  0.5355555555555555,  2.0235294117647054,
            0.6993288590604023,  24.750000000000004,  0.5720496894409937,  0.9159663865546215,  1.7666666666666668]

    def create_models(self):
        for i in range(self.n_clients):
            self.clients_model_list.append(Client(i + 1, self.scheduler, self.model_type, self.path_clients))

    def load_data(self):
        print(f"{self.path_server}/test.pickle")
        test = pd.read_pickle(f"{self.path_server}/test.pickle")

        x_test = test.drop(['label'], axis=1)
        y_test = test['label']

        x_test /= 255.0

        if self.model_type == "CNN":
            x_test = np.array(
                [x.reshape(28, 28) for x in x_test.reset_index(drop=True).values])

        return x_test, y_test

    def aggregate_fit(self, parameters, sample_sizes):

        if self.scheduler != 'FL':
            self.aggregate_fit_cmp(parameters, sample_sizes)
        else:      
            emd_list = np.array(self.clients_emd)[np.array(self.selected_clients) - 1]            
            sample_sizes = sample_sizes * (1 / emd_list)
            self.aggregate_fit_cmp(parameters, sample_sizes)        

    def aggregate_fit_cmp(self, weight_list, sample_sizes):
        self.w_global = []
        for weights in zip(*weight_list):
            weighted_sum = 0
            total_samples = sum(sample_sizes)
            for i in range(len(weights)):
                weighted_sum += weights[i] * sample_sizes[i]
            self.w_global.append(weighted_sum / total_samples)

    def configure_fit(self, selected_clients):
        self.selected_clients = selected_clients

    def fit(self):
        weight_list = []
        sample_sizes_list = []
        info_list = []
        for i, pos in enumerate(self.selected_clients):
            weights, size, info = self.clients_model_list[pos - 1].fit(parameters=self.w_global)
            weight_list.append(weights)
            sample_sizes_list.append(size)
            info_list.append(info)

        return weight_list, sample_sizes_list, {
            "acc_loss_local": [(pos, info_list[i]) for i, pos in enumerate(self.selected_clients)]}

    def centralized_evaluation(self):
        self.model.set_weights(self.w_global)
        loss, accuracy = self.model.evaluate(self.x_test, self.y_test, verbose=False)
        return loss, accuracy
