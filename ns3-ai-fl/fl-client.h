#ifndef FL_CLIENT_H
#define FL_CLIENT_H

#include "ns3/core-module.h"
#include "ns3/fl-global-metrics.h"
#include "ns3/internet-module.h"
#include "ns3/tcp-socket.h"

#include <map>
#include <vector>

namespace ns3 {

class FlClientManager : public Object
{
public:
  static TypeId GetTypeId();

  FlClientManager();
  ~FlClientManager() override = default;
  
  void Configure(Ipv4Address serverIp, uint16_t tcpPort,
                 uint32_t modelPayloadBytes, Time m_roundWindow, Time trainDelay);

  void RegisterNode(Ipv4Address ip, Ptr<Node> node);

  void ConnectForRound(Ipv4Address ip);  

  void ForceCloseAllClientSockets();

  void BeginRound(uint32_t roundIdx) { m_activeRound = roundIdx; m_roundOpen = true; }
  void EndRound() { m_roundOpen = false; }

  struct Ipv4AddrLess {
    bool operator()(const Ipv4Address& a, const Ipv4Address& b) const noexcept {
      return a.Get() < b.Get();
    }
  };

private:
  
  void BuildAndSend(Ptr<Socket> socket, uint32_t iRound, uint32_t payloadBytes) const;
  void ReceivePacketClient(Ptr<Socket> socket);
  void OnNormalCloseClient(Ptr<Socket> socket);
  void OnErrorCloseClient(Ptr<Socket> socket);

  // Estado por conexão (cliente)
  struct ClientState {
    std::vector<uint8_t> buffer;
    uint32_t expectedSize = 0;
    uint32_t iRound = 0;
  };

  struct ClientCloseState { bool closed = false; };

  std::map< Ptr<Socket>, ClientState > m_clientBuffers;   // por-socket
  std::map< Ipv4Address, ClientCloseState, Ipv4AddrLess > m_clientCloseLog;
  std::map< Ipv4Address, Ptr<Node>, Ipv4AddrLess > m_ipToNode;

  Ipv4Address  m_serverIp;
  uint16_t     m_tcpPort = 0;
  uint32_t     m_modelPayloadBytes = 0;
  Time         m_trainDelay = Seconds(0.5);
  Time         m_roundWindow = Seconds(2.1);
  
  uint32_t     m_activeRound = 0;
  bool         m_roundOpen   = false;

};

} // namespace ns3

#endif // FL_CLIENT_H

