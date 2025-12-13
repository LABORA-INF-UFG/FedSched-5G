#include "ns3/core-module.h"
#include "ns3/fl-global-metrics.h"
#include "fl-gym-env.h"
#include <ns3/log.h>
#include <ns3/string.h>
#include <ns3/uinteger.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("fl-gym-env");
NS_OBJECT_ENSURE_REGISTERED(FlGymEnv);

TypeId
FlGymEnv::GetTypeId()
{
  static TypeId tid = TypeId("ns3::FlGymEnv")
      .SetParent<OpenGymEnv>()
      .SetGroupName("Ns3Ai")
      .AddConstructor<FlGymEnv>();
  return tid;
}

FlGymEnv::FlGymEnv()
{
  std::cout << "------------> fl-gym-env.cc" << std::endl;
  SetOpenGymInterface(OpenGymInterface::Get());
}

FlGymEnv::~FlGymEnv() {}

void FlGymEnv::SetServer(Ptr<FlServer> server)
{
  m_server = server;
  if (m_server)
  {
    m_server->onReturnSuccess = [this](uint32_t r, Ipv4Address ip){
      this->OnClientReturn(r, ip);
    };
    m_server->onRoundClosed = [this](uint32_t r){
      this->OnRoundClosed(r);
    };
  }
}

void FlGymEnv::SetClientManager(Ptr<FlClientManager> cliMgr) { m_cliMgr = cliMgr; }

void FlGymEnv::SetIpToNode(const std::map<Ipv4Address, uint32_t, FlClientManager::Ipv4AddrLess>& ip2node)
{
  m_ip2node = ip2node;
  // Constrói vetor indexado por (id-1) -> IP
  m_allIpsByNode.clear();
  m_allIpsByNode.resize(m_ip2node.size());
  for (const auto &kv : m_ip2node) {
    uint32_t id = kv.second;
    if (id >= 1 && id <= m_allIpsByNode.size())
      m_allIpsByNode[id-1] = kv.first;
  }
}

uint32_t FlGymEnv::ObsLen() const
{
  // [phase, round, k] + ueNum ids + [m] + ueNum ids + ueNum SINR
  return 3u + m_ueNum + 1u + m_ueNum + m_ueNum;
}

Ptr<OpenGymSpace> FlGymEnv::GetObservationSpace()
{
  std::vector<uint32_t> shape = { ObsLen() };
  return CreateObject<OpenGymBoxSpace>(-1.0e12, 1.0e12, shape, TypeNameGet<double>());
}

Ptr<OpenGymSpace> FlGymEnv::GetActionSpace()
{
  std::vector<uint32_t> shape = { 3u + m_ueNum }; // phase, round, k, ids[k] (resto 0)
  return CreateObject<OpenGymBoxSpace>(0.0, 1.0e9, shape, TypeNameGet<uint32_t>());
}

Ptr<OpenGymDataContainer> FlGymEnv::GetObservation()
{
  const uint32_t L = ObsLen();
  std::vector<uint32_t> shape = { L };
  auto box = CreateObject<OpenGymBoxContainer<double>>(shape);
  
  double phase = 1.0;
  box->AddValue(phase);
  box->AddValue((double)m_roundIdx);
  box->AddValue((double)(m_lastSelectedIds.size()));

  // ids selecionados
  for (uint32_t i = 0; i < m_ueNum; ++i) {
    double v = -1.0;
    if (i < m_lastSelectedIds.size()) v = (double)m_lastSelectedIds[i];
    box->AddValue(v);
  }

  box->AddValue((double)m_successIds.size());

  // ids com sucesso
  for (uint32_t i = 0; i < m_ueNum; ++i) {
    double v = -1.0;
    if (i < m_successIds.size()) v = (double)m_successIds[i];
    box->AddValue(v);
  }

  // SINR (dB) por UE
  for (uint32_t i = 0; i < m_ueNum; ++i) {
    double val = 0.0;
    if (i < m_sinrDb.size()) val = m_sinrDb[i];
    box->AddValue(val);
  }

  return box;
}

bool FlGymEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
  auto box = DynamicCast<OpenGymBoxContainer<uint32_t>>(action);
  if (!box) return true;

  uint32_t roundIdx = box->GetValue(1);
  uint32_t k        = box->GetValue(2);
  std::vector<uint32_t> ids;
  ids.reserve(k);
  for (uint32_t i = 0; i < k; ++i) {
    ids.push_back(box->GetValue(3 + i));
  }

  StartRoundFromSelection(roundIdx, ids);
  return true;
}

void FlGymEnv::StartRoundFromSelection(uint32_t roundIdx,
                                       const std::vector<uint32_t>& selIds)
{
  if (!m_server || !m_cliMgr) return;

  fl::GlobalMetrics::Instance().IncRound();
  
  m_roundIdx = roundIdx;
  m_lastSelectedIds = selIds;
  m_successIds.clear();

  m_cliMgr->BeginRound(roundIdx);

  std::vector<Ipv4Address> participants;
  participants.reserve(selIds.size());
  for (uint32_t id : selIds) {
    if (id >= 1 && id <= m_allIpsByNode.size()) {
      participants.push_back(m_allIpsByNode[id-1]);
    }
  }

  Time now = Simulator::Now();
  Time startT = now < Seconds(1.0) ? Seconds(1.0) : now;

  Simulator::Schedule(startT - now, [this, roundIdx, participants]() {
    m_server->StartRound(roundIdx, participants);
    for (const auto &ip : participants) {
      Simulator::ScheduleNow(&FlClientManager::ConnectForRound, m_cliMgr, ip);
    }
  });
}

void FlGymEnv::OnClientReturn(uint32_t /*roundIdx*/, Ipv4Address ip)
{
  auto it = m_ip2node.find(ip);
  uint32_t id = 0u;
  if (it != m_ip2node.end()) id = it->second;

  m_successIds.push_back(id);
  fl::GlobalMetrics::Instance().AddSuccessId(id);
}

void FlGymEnv::OnRoundClosed(uint32_t roundIdx)
{
  if (m_cliMgr) {
    m_cliMgr->ForceCloseAllClientSockets();
    // Marcar rodada encerrada no cliente
    m_cliMgr->EndRound();
  }

  // Notifica Python com a observação da rodada
  Notify();

  // Quando todas as rodadas finalizarem
  if (roundIdx + 1 >= m_nRounds) {
    m_done = true;
  }
}

void FlGymEnv::SetSinrVector(const std::vector<double>& sinrDb)
{
  m_sinrDb = sinrDb;
  if (m_sinrDb.size() < m_ueNum) m_sinrDb.resize(m_ueNum, 0.0);
  if (m_sinrDb.size() > m_ueNum) m_sinrDb.resize(m_ueNum);
}
} // namespace ns3
