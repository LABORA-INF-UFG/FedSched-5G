import numpy as np
import pandas as pd
import ot

class UeMetrics:
    def __init__(self, ue_num, alpha=8, beta=1):
        self.suporte = np.arange(10)
        self.ue_num = ue_num    
        self.alpha = alpha
        self.beta = beta        
        self.user_emd = [] 

        self.user_emd_aux = [
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
        
        self.init()

    @staticmethod
    def group_map(ind):
        map_list = [
            5, 5, 3, 4, 2, 
            1, 3, 5, 5, 4, 
            3, 3, 3, 5, 1, 
            4, 1, 2, 2, 1, 
            5, 4, 5, 3, 4, 
            5, 4, 3, 2, 1, 
            4, 1, 3, 1, 3, 
            1, 2, 5, 3, 2, 
            1, 4, 1, 4, 1, 
            2, 3, 5, 2, 4, 
            5, 1, 4, 2, 2, 
            3, 1, 5, 2, 2, 
            5, 5, 3, 5, 4, 
            3, 1, 4, 5, 2, 
            4, 5, 5, 1, 3, 
            1, 4, 1, 1, 5, 
            4, 5, 4, 3, 1, 
            2, 5, 4, 5, 3, 
            1, 2, 1, 3, 1, 
            2, 5, 4, 1, 1, 
            3, 5, 1, 2, 3, 
            4, 4, 2, 2, 4, 
            3, 3, 2, 3, 2, 
            5, 3, 2, 2, 2, 
            1, 4, 2, 3, 3, 
            1, 2, 3, 2, 5, 
            4, 1, 2, 5, 5, 
            2, 4, 5, 3, 2, 
            5, 1, 4, 4, 3, 
            4, 1, 4, 4, 3]
        return map_list[ind]

    def init(self): 
        
        """
        for i in range(self.ue_num):
             cid = i + 1
             df = pd.read_pickle(f"/home/lenav40/Documentos/Datasets/mnist/ds/{cid}_train.pickle")
             emd_value = self.calcular_emd_penalizado(df.label)           
             self.user_emd.append(emd_value)          
        """
       
        self.user_emd = self.user_emd_aux.copy()
        print(f"EMD: {self.user_emd}")       
        
    
    def min_max_normalize(self, min_val, max_val, value):
        if max_val == min_val:           
            return 0.0
        return (value - min_val) / (max_val - min_val)

    def update(self, ids, sinr):               
        ue_metric = []
        for idx, cid in enumerate(ids):
            
            emd_value = self.min_max_normalize(
                np.min(self.user_emd),
                np.max(self.user_emd),
                self.user_emd[cid-1]
            )
            snr_value = self.min_max_normalize(
                np.min(1 / np.array(sinr)),
                np.max(1 / np.array(sinr)),
                1 / sinr[cid-1]
            )

            combined_value = self.alpha * emd_value + self.beta * snr_value            
            ue_metric.append(combined_value)
            print(f"cid: {cid} -> sinr: {sinr[cid-1]}/{snr_value} -> emd: {self.user_emd[cid-1]}/{emd_value} -> combined_value: {combined_value} -> Group: {UeMetrics.group_map(cid - 1)}")

        return ue_metric.copy()


    def calcular_histograma(self, v_labels):
        contagens = np.array([np.sum(v_labels == rotulo) for rotulo in self.suporte ])
        total = contagens.sum()
        if total == 0:
            return np.zeros_like(contagens, dtype=float)
        return contagens / total

    def definir_matriz_custo(self, tipo='numerico'):
        n = len(self.suporte )
        C = np.zeros((n, n))
        for i in range(n):
            for j in range(n):
                if tipo == 'numerico':
                    C[i, j] = abs(self.suporte [i] - self.suporte [j])
                else:  # caso categórico
                    C[i, j] = 0 if self.suporte [i] == self.suporte [j] else 1
        return C

    def calcular_emd_penalizado(self, v_labels, gamma=5.0):
        
        P = self.calcular_histograma(v_labels)      
        n = len(self.suporte )
        U = np.ones(n) / n
      
        C = self.definir_matriz_custo()
        
        emd_value = ot.emd2(P, U, C)       
        missing_bins = np.sum(P == 0)
        penalty_factor = 1 + gamma * (missing_bins / n)        
        emd_final = emd_value * penalty_factor
        return emd_final

