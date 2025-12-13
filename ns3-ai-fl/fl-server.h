#ifndef FL_SERVER_H
#define FL_SERVER_H

#include "ns3/core-module.h"
#include "ns3/fl-global-metrics.h"
#include "ns3/internet-module.h"
#include "ns3/tcp-socket.h"

#include <map>
#include <set>
#include <vector>
#include <functional>

namespace ns3 {

class FlServer : public Object
{
public:
  static TypeId GetTypeId();

  FlServer();
  ~FlServer() override = default;

  void Configure(Ptr<Node> remoteHost, Ipv4Address serverIp,
                 uint16_t tcpPort, uint32_t modelPayloadBytes,
                 Time roundWindow, uint32_t nRounds);
  
  void BindAndListen();
  void StartRound(uint32_t roundIdx, const std::vector<Ipv4Address>& participants);

  std::function<void(uint32_t /*nextIdx*/, bool /*hasMore*/)> onAdvance;
  std::function<void(uint32_t /*roundIdx*/, Ipv4Address /*clientIp*/)> onReturnSuccess;
  std::function<void(uint32_t /*roundIdx*/)> onRoundClosed;

  const std::vector<Ipv4Address>& GetParticipants() const { return m_roundParticipants; }

private:
  void BuildAndSend(Ptr<Socket> socket, uint32_t iRound, uint32_t payloadBytes) const;
  void SendGlobalToSocket(Ptr<Socket> sock);

  void ResetPerRoundControllers();
  void CancelRoundTimer();
  void AggregateAndAdvance();

  uint32_t CountReturns() const;

  void AcceptConnection(Ptr<Socket> newSocket, const Address& from);
  void ReceivePacketServer(Ptr<Socket> socket);
  void OnNormalCloseServer(Ptr<Socket> s);
  void OnErrorCloseServer(Ptr<Socket> s);
  void OnRoundTimer();

  struct BufferState {
    std::vector<uint8_t> buffer;
    uint32_t expectedSize = 0;
    uint32_t iRound = 0;
    bool gotReturn = false;
    bool closedLogged = false;
  };

  std::map< Ptr<Socket>, BufferState > m_serverBuffers;

  Ptr<Node>   m_remoteHost;
  Ipv4Address m_serverIp;
  uint16_t    m_tcpPort = 0;
  uint32_t    m_modelPayloadBytes = 0;

  uint32_t    m_round = 0;
  uint32_t    m_nRounds = 0;
  bool        m_roundStarted = false;

  Time        m_roundWindow = Seconds(6.0);
  EventId     m_roundEv;
  bool        m_timerOpen = false;

  std::vector<Ipv4Address> m_roundParticipants;

  Ptr<Socket> m_serverSock;
};

} // namespace ns3

#endif // FL_SERVER_H

