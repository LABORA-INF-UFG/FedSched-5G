#ifndef NS3_FL_GYM_ENV_H
#define NS3_FL_GYM_ENV_H

#include <ns3/ai-module.h>
#include <ns3/core-module.h>
#include "ns3/fl-global-metrics.h"
#include <ns3/internet-module.h>

#include "fl-client.h"
#include "fl-server.h"

#include <vector>
#include <map>

namespace ns3 {

class FlGymEnv : public OpenGymEnv
{
public:
  static TypeId GetTypeId();
  FlGymEnv(); 
  ~FlGymEnv() override;

  Ptr<OpenGymSpace> GetObservationSpace() override;
  Ptr<OpenGymSpace> GetActionSpace() override;
  Ptr<OpenGymDataContainer> GetObservation() override;
  bool ExecuteActions(Ptr<OpenGymDataContainer> action) override;
  bool GetGameOver() override { return m_done; }
  float GetReward() override { return 0.0f; }
  std::string GetExtraInfo() override { return std::string(); }

  void SetServer(Ptr<FlServer> server);
  void SetClientManager(Ptr<FlClientManager> cliMgr);
  void SetIpToNode(const std::map<Ipv4Address, uint32_t, FlClientManager::Ipv4AddrLess>& ip2node);

  void SetUeNum(uint32_t ueNum) { m_ueNum = ueNum; m_sinrDb.assign(m_ueNum, 0.0); }
  void SetKPerRound(uint32_t k) { m_kPerRound = k; }
  void SetNumRounds(uint32_t n) { m_nRounds = n; }

  void SetRoundWindow(Time w) { m_roundWindow = w; }
  void SetSinrVector(const std::vector<double>& sinrDb);

  void OnClientReturn(uint32_t roundIdx, Ipv4Address ip);
  void OnRoundClosed(uint32_t roundIdx);

private:

  void StartRoundFromSelection(uint32_t roundIdx,
                               const std::vector<uint32_t>& selIds);

  Ptr<FlServer> m_server;
  Ptr<FlClientManager> m_cliMgr;
  std::map<Ipv4Address, uint32_t, FlClientManager::Ipv4AddrLess> m_ip2node;
  std::vector<Ipv4Address> m_allIpsByNode; 

  uint32_t m_ueNum{0};
  uint32_t m_kPerRound{0};
  uint32_t m_roundIdx{0};
  uint32_t m_nRounds{0};
  bool     m_done{false};

  Time     m_roundWindow{Seconds(6.0)};

  std::vector<uint32_t>  m_lastSelectedIds;
  std::vector<uint32_t>  m_successIds;

  std::vector<double>   m_sinrDb;

  uint32_t ObsLen() const;
};

} // namespace ns3

#endif
