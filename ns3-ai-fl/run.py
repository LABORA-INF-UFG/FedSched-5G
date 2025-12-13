import argparse
import gymnasium as gym
import ns3ai_gym_env
from agents import FLSelector
import numpy as np
from uemetrics import UeMetrics
import warnings

def pack_action(round_idx, sel_ids, ue_num):  
    k = len(sel_ids)
    vec = [0, int(round_idx), int(k)]
    ids_pad = [int(x) for x in sel_ids] + [0] * (ue_num - k)
    vec.extend(ids_pad)
    return np.asarray(vec, dtype=np.uint32)


def parse_observation(obs, ue_num):
    obs = np.asarray(obs, dtype=np.float64)

    r_idx = int(obs[1])
    k_sel = int(obs[2])

    base_sel = 3
    selected = [int(x) for x in obs[base_sel: base_sel + ue_num] if int(x) > 0]

    base_m = base_sel + ue_num
    m_succ = int(obs[base_m])
    succ_ids = [int(x) for x in obs[base_m + 1: base_m + 1 + ue_num] if int(x) > 0]

    base_sinr = base_m + 1 + ue_num
    sinr_vec = [float(x) for x in obs[base_sinr: base_sinr + ue_num]]

    return r_idx, selected[:k_sel], succ_ids[:m_succ], sinr_vec


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ueNum", type=int, default=150, help="Número total de UEs") 
    parser.add_argument("--kPerRound", type=int, default=6, help="Número de UEs por rodada")
    parser.add_argument("--scheduler", type=str, default="FL", help="Scheduler")
    parser.add_argument("--nRounds", type=int, default=100, help="Número de rodadas")
    parser.add_argument("--roundWindow", type=float, default=5, help="Janela de tempo por rodada (s)") # 6
    parser.add_argument("--trainDelay", type=float, default=0.5, help="Delay de treinamento (s)")
    parser.add_argument("--payloadBytes", type=int, default=681892, help="payloadBytes") # 407080
    parser.add_argument("--enableExtraApp", type=int, default=1, help="Habilita extraApp")   
    parser.add_argument("--seed", type=int, default=15, help="Seed")     
    parser.add_argument("--ns3_path", type=str, default="../../../../", help="Caminho para o ns-3")    
    args = parser.parse_args()

    ns3Settings = {
        "ueNum": args.ueNum,
        "uePerRound": args.kPerRound,
        "n_rounds": args.nRounds,
        "roundWindow": args.roundWindow,
        "trainDelay": args.trainDelay,
        "scheduler": args.scheduler,
        "enableExtraApp": args.enableExtraApp,
        "payloadBytes": args.payloadBytes,       
        "seed": args.seed,  
        "simTime": args.nRounds * args.roundWindow * 2
    }

    env = gym.make(
        "ns3ai_gym_env/Ns3-v0",
        targetName="ns3ai_fl_gym",
        ns3Path=args.ns3_path,
        ns3Settings=ns3Settings,
        disable_env_checker=True,
    )
    

    obs, info = env.reset()
    if obs is None:
        L = 3 + args.ueNum + 1 + args.ueNum + args.ueNum
        obs = np.zeros(L, dtype=np.float32)

    enableRandomUser = 0 if args.scheduler == "FL" else 1
    selector = FLSelector(args.ueNum, args.kPerRound, enableRandomUser, args.seed)

    # Observação inicial
    obs = np.asarray(obs, dtype=np.float32)
    r_idx_obs, selected_echo, succ_ids_prev, sinr_vec = parse_observation(obs, args.ueNum)
    print(f"SINR: {sinr_vec}")
    print(f"enableRandomUser: {enableRandomUser}")

    for r in range(args.nRounds):        

        sel_ids = selector.select_for_round(sinr_vec)
        # Ordenar os sel_ids pelo valor de SINR correspondente
        sel_ids_sorted = sorted(sel_ids, key=lambda i: sinr_vec[i - 1], reverse=True)

        # Reconstruir o vetor de SINR já ordenado
        selected_sinr = [sinr_vec[i - 1] for i in sel_ids_sorted]
        selected_group = [UeMetrics.group_map(i - 1) for i in sel_ids_sorted]
        print(f"[Python] Round {r+1}: selecionados ordenados: {sel_ids_sorted}")
        print(f"[Python] Group: {selected_group}")
        print(f"[Python] SINR: {selected_sinr}")      

        act = pack_action(r, sel_ids_sorted, args.ueNum)
        obs, reward, done, _, info = env.step(act)
        obs = np.asarray(obs, dtype=np.float32)  

        r_idx, selected_echo, succ_ids = parse_observation(obs, args.ueNum)[:3]
        print(f"[Python] Round {r_idx+1}: sucesso (ids) = {succ_ids}\n")

        if done: 
            break

    env.close()


if __name__ == "__main__":
    print(f"------------> run.py")
    main()
