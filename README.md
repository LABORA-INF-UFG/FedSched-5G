# Aprendizado Federado em Redes 5G: Algoritmo de Escalonamento de Recursos para o Treinamento Federado Baseado no Módulo 5G-LENA do ns-3
Este trabalho propõe um algoritmo de escalonamento, denominado FedSched (*Federated Learning-Aware Resource Scheduler*), voltado para tarefas de Aprendizado Federado em redes 5G, onde os fluxos de atualização de modelo competem com tráfego de fundo interferente. O algoritmo seleciona dispositivos com dados mais representativos e com melhores condições de canal, emprega uma heurística de agendamento que prioriza os fluxos de atualização de modelo com coexistência de tráfego de fundo e aplica uma estratégia de agregação que reforça as contribuições que favorecem a convergência do modelo global. A implementação é integrada ao módulo 5G-LENA do ns-3 e demonstra melhorias na convergência e nos indicadores de desempenho da rede 5G, superando os algoritmos tradicionais de escalonamento.

---

## 🎯 Contribuições
 
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
