# Escalonamento de Recursos de Rádio para Suporte ao Aprendizado Federado em Redes 5G
Este trabalho propõe um algoritmo de escalonamento, denominado FedSched (*Federated Learning-Aware Resource Scheduler*), voltado para tarefas de Aprendizado Federado em redes 5G, onde os fluxos de atualização de modelo competem com tráfego de fundo concorrente não prioritário. O algoritmo seleciona dispositivos com dados mais representativos e com melhores condições de canal, emprega uma heurística de agendamento que prioriza os fluxos de atualização de modelo com coexistência de tráfego de fundo e aplica uma estratégia de agregação que reforça as contribuições que favorecem a convergência do modelo global. A implementação é integrada ao módulo 5G-LENA do ns-3 e demonstra melhorias na convergência e nos indicadores de desempenho da rede 5G, superando os algoritmos tradicionais de escalonamento.

---

# 🎯 Contribuições
 
- **Integração com Módulo 5G-LENA do ns-3:**  
FedSched estende o escalonador OFDMA do módulo 5G-LENA do ns-3 sem alterar sua arquitetura.

- **Seleção de UEs:**  
FedSched seleciona UEs combinando os valores de SINR e EMD para identificar os UEs mais relevantes em cada rodada.

- **Controle Adaptativo de Coexistência:**  
FedSched utiliza uma política de controle que ajusta dinamicamente a presença de tráfego de fundo, permitindo a coexistência proporcional em função da quantidade de fluxos de atualização de modelo.

- **Fila de Agendamento Prioritário:**  
FedSched prioriza os fluxos de UEs FL com maior MCS e assegura tratamento equitativo em caso de empate.

- **Agregação Ponderada pelo EMD:** 
FedSched utiliza um esquema de agregação que reforça contribuições de UEs com melhores dados para a convergência de $w_{global}$.

- **Novo Mecanismo de Coleta de Métricas:** 
FedSched introduz a classe GlobalMetrics, uma instância única que registra as decisões do escalonador e possibilita a estruturação das métricas do FlowMonitor por rodada de comunicação. Além do mais, GlobalMetrics pode ser utilizada para a geração de datasets da simulação de rede. 

- **Disponibilização da Implementação:** 
O código de FedSched é disponibilizado, permitindo a reprodução dos experimentos, a validação dos resultados e o uso como base para estudos e extensões futuras.


# Estrutura do readme.md

O repositório está organizado em quatro diretórios:

- **ds:** Responsável pela geração do dataset utilizado nos experimentos.

- **nr:** Contém as modificações no módulo 5G-LENA, incluindo a implementação do escalonador Fed-Sched.

- **ns3-ai-fl:** Realiza a simulação de rede no ns-3 e gera métricas da simulação de rede.

- **FL:** Executa a simulação do FL com base nas métricas obtidas na rede.

# Selos Considerados

- Artefatos Disponíveis (SeloD)  
- Artefatos Funcionais (SeloF)
- Artefatos Sustentáveis (SeloS)  
- Experimentos Reprodutíveis (SeloR)
  
Com base nos códigos e documentação disponibilizados neste e nos repositórios relacionados.

# Dependências
Esta seção descreve os requisitos necessários para execução do projeto.

## Requisitos

- Sistema Operacional: Ubuntu 20.04 LTS
- Kernel Linux: 5.x
- Processador: Intel Core i7
- Memória RAM: mínimo de 8 GB
- GCC/G++: 11.4.0
- Python: 3.10
- Conda
- ns-3: versão 3.44  
- 5G-LENA: branch `5g-lena-v4.0.y`
- ns3-ai
  
As principais bibliotecas utilizadas no ambiente experimental incluem:

- `tensorflow==2.10.0`
- `keras==3.13.1`
- `tensorboard==2.20.0`
- `protobuf==3.20.3`
- `gym==0.26.2`
- `gymnasium==1.2.3`
- `numpy==1.26.4`
- `pandas==3.0.0`
- `matplotlib==3.10.8`
  
O ambiente experimental do ns-3 e do módulo 5G-LENA depende das seguintes ferramentas e bibliotecas:

- GCC/G++ 11.4.0
- Bibliotecas padrão do ns-3.44
- Bibliotecas do módulo 5G-LENA v4.0
- Integração C++/Python do ns3-ai


## Ambiente Experimental

# Preocupações com segurança
A execução deste artefato é isenta de riscos para os avaliadores. Não há necessidade de operações que possam comprometer o sistema.

# Guia de Reprodução

Esta seção apresenta um roteiro linear, autocontido e reproduzível para execução completa do artefato experimental do FedSched. Todos os passos devem ser executados na sequência indicada abaixo.

## Fluxo de Reprodução

A reprodução completa do artigo segue a sequência abaixo:

1. Clonagem do repositório principal;
2. Instalação do ns-3;
3. Instalação do módulo 5G-LENA;
4. Instalação do módulo ns3-ai;
5. Integração dos arquivos do FedSched ao ns-3;
6. Geração dos datasets locais dos clientes;
7. Compilação do ambiente ns-3;
8. Execução do teste mínimo;
9. Execução da simulação de rede;
10. Execução do treinamento federado;
11. Análise dos resultados gerados em `fl-results/`.

## Observações de Reprodutibilidade

A reprodução completa dos experimentos depende da execução correta de todas as etapas de instalação descritas neste README, incluindo: obtenção do código-fonte, geração do dataset, instalação do ns-3, instalação dos módulos 5G-LENA e ns3-ai, bem como a integração do projeto FedSched ao ambiente de simulação.

Em especial, os procedimentos de instalação do ns-3, do módulo 5G-LENA e do módulo ns3-ai seguem as instruções descritas em suas respectivas documentações oficiais, referenciadas nas seções correspondentes deste README.

## Resultado Esperado

Ao final da execução completa do fluxo experimental, espera-se que o diretório `fl-results/` contenha:

```text
fl-results/
├── seed<id>/
│   ├── FL/
│   ├── RR/
│   ├── PF/
│   └── MR/
└── seed-avg/
```

# Instalação

## Obtenção do Código-Fonte

Clone o repositório principal do projeto:

```bash
git clone https://github.com/LABORA-INF-UFG/FedSched-5G.git
cd FedSched-5G
```

## Geração do Dataset

Antes da simulação, gere os dados locais dos clientes. Para o dataset MNIST, utilize:
```bash
cd FedSched-5G/ds
python generate_dataset.py --dataset_name mnist --num_clients 150
```
Para o Fashion-MNIST, utilize o parâmetro fashion_mnist:
```bash
cd FedSched-5G/ds
python generate_dataset.py --dataset_name fashion_mnist --num_clients 150
```


## NR ns-3 module
Para verificar as dependências necessárias, requisitos de compilação e instruções adicionais de instalação do módulo 5G-LENA, consulte a documentação oficial em: https://github.com/QiuYukang/5G-LENA/tree/5g-lena-v4.0.y

### ns-3
Faça o download do simulador ns-3 a partir do repositório oficial e selecione a versão 3.44, utilizada neste projeto.

```bash
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
git checkout -b ns-3.44 ns-3.44
```

### 5G-LENA
Adicione o módulo 5G-LENA ao diretório contrib do ns-3, utilizando a versão v4.0.

```bash
cd ns3_directory/contrib
git clone https://gitlab.com/cttc-lena/nr.git
cd nr
git checkout -b 5g-lena-v4.0.y origin/5g-lena-v4.0.y
```

No diretório `ns-3-dev`, compile o ambiente e execute um exemplo para validar a instalação.

```bash
cd ns3_directory
./ns3 configure --enable-examples --enable-tests
./ns3 build
./ns3 run cttc-nr-demo
```

## ns3-ai module
Para verificar as dependências necessárias, requisitos de compilação e instruções adicionais de instalação do módulo ns3-ai, consulte a documentação oficial em: https://github.com/hust-diangroup/ns3-ai/blob/main/docs/install.md

### ns3-ai
Crie um ambiente virtual Conda dedicado ao ns3-ai.
```bash
conda create -n ns3ai_env python=3.10 tensorflow==2.10.0 -y
conda activate ns3ai_env
pip install pandas matplotlib POT
```

Adicione o módulo ns3-ai ao diretório contrib do ns-3.
```bash
cd ns3_directory
git clone https://github.com/hust-diangroup/ns3-ai.git contrib/ai
```

Construa a biblioteca `ai` e configurare as interfaces Python.
```bash
./ns3 configure --enable-examples -- -DPython_EXECUTABLE=<path-to-python> -DPython3_EXECUTABLE=<path-to-python>
./ns3 build ai
pip install -e contrib/ai/python_utils
pip install -e contrib/ai/model/gym-interface/py
```

# Integração do Projeto
Para que o Fed-Sched funcione corretamente, é necessário integrar os arquivos deste repositório ao módulo NR do ns-3.

Os arquivos deste repositório devem modificar o módulo 5G-LENA.
```bash
cp -r FedSched-5G/nr/model/* ns3_directory/contrib/nr/model/
cp -r FedSched-5G/nr/helper/* ns3_directory/contrib/nr/helper/
```

O diretório  `ns3-ai-fl ` contém o cenário e as aplicações de simulação e deve ser copiado para o diretório de exemplos do ns3-ai.
```bash
mkdir -p ns-3-dev/contrib/ai/examples/ns3-ai-fl
cp -r FedSched-5G/ns3-ai-fl/* ns3_directory/contrib/ai/examples/ns3-ai-fl/
```

Antes de executar os experimentos, é necessário criar o diretório `fl-results/`, que armazenará os resultados organizados por seed e por scheduler, conforme o exemplo abaixo.
```bash
mkdir -p ns3_directory/fl-results/seed1/{FL,RR,MR,PF}
mkdir -p ns3_directory/fl-results/seed-avg
```

# Teste Mínimo

O teste mínimo da simulação de rede no `ns3` utilizando o escalonador `RR`. Espera-se que a simulação seja executada sem erros e que os arquivos de saída sejam gerados em `ns3_directory/fl-results/seed1/RR`.

```bash 
conda activate ns3ai_env
cd ns3_directory/contrib/ai/examples/ns3-ai-fl
python ./run.py --scheduler=RR --nRounds=5 --seed=1
```

# Experimentos

## Parâmetros na Reprodutibilidade dos Resultados

Os resultados obtidos pelo FedSched dependem diretamente da configuração dos parâmetros da simulação de rede e do treinamento federado. Pequenas alterações nesses parâmetros podem modificar o comportamento do escalonador, a coexistência entre os fluxos de rede e a convergência do modelo global.

No ambiente de rede, parâmetros como número de UEs (`--ueNum`), quantidade de dispositivos selecionados por rodada (`--kPerRound`), duração da janela de comunicação (`--roundWindow`), atraso de treinamento local (`--trainDelay`) e tamanho do modelo transmitido (`--payloadBytes`) influenciam diretamente o uso de recursos de rádio, throughput, atraso, taxa de entrega e quantidade de atualizações FL concluídas com sucesso.

Da mesma forma, a ativação de tráfego concorrente (`--enableExtraApp`) altera o nível de contenção no uplink, impactando o comportamento relativo entre os escalonadores RR, PF, MR e FedSched. Assim, diferentes cargas de tráfego podem produzir diferenças significativas nos indicadores de QoS e na taxa de sucesso das rodadas de Aprendizado Federado.

No treinamento federado, parâmetros como número de rodadas (`--nRounds`), quantidade de clientes, tipo de modelo (`MLP` ou `CNN`) e distribuição dos dados locais também influenciam diretamente a convergência e a estabilidade do modelo global.

## Reivindicação #1: Simulação de Rede ##

Esta etapa executa a simulação de rede no ns-3 utilizando o escalonador Fed-Sched. Outros parâmetros adicionais da simulação de rede  podem ser ajustados diretamente no arquivo `sim.cc`.

```bash
conda activate ns3ai_env
cd ns3_directory/contrib/ai/examples/ns3-ai-fl
python ./run.py \
  --ueNum=150 \
  --kPerRound=6 \
  --scheduler=FL \
  --nRounds=100 \
  --roundWindow=5 \
  --trainDelay=0.5 \
  --payloadBytes=681892 \
  --enableExtraApp=1 \
  --seed=1
```

- `--ueNum`: número total de dispositivos (UEs) na simulação.
- `--kPerRound`: número de UEs selecionados por rodada de FL.
- `--scheduler`: escalonador utilizado (FL, RR, PF ou MR).
- `--nRounds`: número total de rodadas de FL.
- `--roundWindow`: duração (em segundos) de cada rodada.
- `--trainDelay`: tempo de processamento local do modelo em cada UE.
- `--payloadBytes`: tamanho do modelo transmitido em bytes.
- `--enableExtraApp`: habilita tráfego adicional concorrente na rede.
- `--seed`: semente aleatória para reprodutibilidade.


## Reivindicação #2: Simulação de FL ##

Esta etapa executa a simulação de FL utilizando os resultados gerados pela simulação de rede.

```bash
conda activate ns3ai_env
cd ns3_directory/contrib/ai/examples/ns3-ai-fl
python run.py \
  --scheduler FL \
  --n_seed=1 \
  --n_clients 150 \
  --n_rounds 100 \
  --model_type MLP \
  --path_clients ./ds_directory/mnist \
  --path_server ./ds_directory/mnist/mnist \
  --path_fl_results ./ns3_directory/contrib/ai/examples/ns3-ai-fl
```
- `--scheduler`: escalonador utilizado (FL, RR, PF ou MR), que define quais UEs participam de cada rodada.
- `--n_seed`: número de seeds utilizadas na simulação de rede.
- `--n_clients`: número total UEs no treinamento federado.
- `--n_rounds`: número de rodadas de treinamento federado.
- `--model_type`: tipo de modelo utilizado (ex: MLP ou CNN).
- `--path_clients`: caminho para os dados locais dos clientes.
- `--path_server`: caminho para o dataset global utilizado na avaliação.
- `--path_fl_results`: caminho onde estão os resultados da simulação de rede.

# LICENSE
Este projeto está licenciado sob a licença Creative Commons Attribution 4.0 International (CC BY 4.0).
