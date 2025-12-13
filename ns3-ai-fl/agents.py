import random
import pandas as pd
from uemetrics import UeMetrics

class FLSelector:
  
    def __init__(self, ue_num, k_per_round, enableRandomUser, seed):
        print(f"------------> agents.py")
        self.ue_num = int(ue_num)
        self.k = int(k_per_round)
        self.enableRandomUser = enableRandomUser
        self.seed = seed
        self.factor_per_round = int(k_per_round * 4)
        self.ueMetrics = UeMetrics(self.ue_num)        
        random.seed(self.seed)
    
    def random_user_selection(self):
        ids = list(range(1, self.ue_num + 1))
        random.shuffle(ids)
        return ids[: self.k] 
    
    def user_selection(self, sinr_vec):
        ids = list(range(1, self.ue_num + 1))
        random.shuffle(ids) 

        ids_factor = ids[: self.factor_per_round]
        print(f"ids_factor: {ids_factor}")
        metrics_list = self.ueMetrics.update(ids_factor, sinr_vec)
        print(f"metrics_list: {metrics_list}")        

        ids_final = [i for i, _ in sorted(
            zip(ids_factor, metrics_list),  
            key=lambda x: x[1],             
            reverse=False
        )]
        print(f"ids_final: {ids_final}")

        return ids_final[: self.k] 

    def select_for_round(self, sinr_vec):         
        if self.enableRandomUser:
            ids = self.random_user_selection()
        else:
            ids = self.user_selection(sinr_vec)       
        return ids 