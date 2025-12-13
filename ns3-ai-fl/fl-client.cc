#include "ns3/core-module.h"
#include "ns3/fl-global-metrics.h"
#include "fl-client.h"
#include <cstring>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(FlClientManager);

TypeId FlClientManager::GetTypeId()
{
  static TypeId tid = TypeId("ns3::FlClientManager")
    .SetParent<Object>()
    .SetGroupName("Applications")
    .AddConstructor<FlClientManager>();
  return tid;
}

FlClientManager::FlClientManager() {
  std::cout << "------------> fl-client.cc" << std::endl;
}

void
FlClientManager::Configure(Ipv4Address serverIp, uint16_t tcpPort,
                           uint32_t modelPayloadBytes, Time roundWindow, Time trainDelay)
{
  m_serverIp = serverIp;
  m_tcpPort = tcpPort;
  m_modelPayloadBytes = modelPayloadBytes;
  m_roundWindow = roundWindow;
  m_trainDelay = trainDelay;
}

void
FlClientManager::RegisterNode(Ipv4Address ip, Ptr<Node> node)
{
  m_ipToNode[ip] = node;
}

void
FlClientManager::BuildAndSend(Ptr<Socket> socket, uint32_t iRound, uint32_t payloadBytes) const
{
  std::vector<uint8_t> payload(payloadBytes, 0);
  const uint32_t tamanho = payload.size();

  std::vector<uint8_t> buffer(sizeof(uint32_t) * 2 + tamanho);
  std::memcpy(buffer.data(), &tamanho, sizeof(uint32_t));
  std::memcpy(buffer.data() + sizeof(uint32_t), &iRound, sizeof(uint32_t));
  if (tamanho) {
    std::memcpy(buffer.data() + sizeof(uint32_t) * 2, payload.data(), tamanho);
  }

  Ptr<Packet> pkt = Create<Packet>(buffer.data(), buffer.size());
  socket->Send(pkt);
}

void
FlClientManager::ReceivePacketClient(Ptr<Socket> socket)
{
  while (Ptr<Packet> packet = socket->Recv())
  {
    const uint32_t sz = packet->GetSize();
    if (sz == 0) {
      Simulator::ScheduleNow(&Socket::Close, socket);
      return;
    }

    auto &state = m_clientBuffers[socket];
    std::vector<uint8_t> tmp(sz);
    packet->CopyData(tmp.data(), sz);
    state.buffer.insert(state.buffer.end(), tmp.begin(), tmp.end());

    while (true) {
      auto &buf = state.buffer;
      if (state.expectedSize == 0) {
        if (buf.size() < sizeof(uint32_t) * 2) break;
        std::memcpy(&state.expectedSize, buf.data(), sizeof(uint32_t));
        std::memcpy(&state.iRound, buf.data() + sizeof(uint32_t), sizeof(uint32_t));
      }
      if (buf.size() < sizeof(uint32_t) * 2 + state.expectedSize) break;
      
      if (!m_roundOpen || state.iRound != m_activeRound) {
        Simulator::ScheduleNow(&Socket::Close, socket);        
        buf.erase(buf.begin(), buf.begin() + sizeof(uint32_t) * 2 + state.expectedSize);
        state.expectedSize = 0;
        break;
      }

      Address peerAddr; socket->GetPeerName(peerAddr);
      Ipv4Address ipServer = InetSocketAddress::ConvertFrom(peerAddr).GetIpv4();
      Address localAddr; socket->GetSockName(localAddr);
      Ipv4Address ipClient = InetSocketAddress::ConvertFrom(localAddr).GetIpv4();

      std::cout << Simulator::Now().As(Time::S)
                << " [R:" << (state.iRound+1) << "] [C] " << ipClient
                << " recebeu w_global (" << (sizeof(uint32_t)*2 + m_modelPayloadBytes) << " bytes) de " << ipServer
                << " [treinamento local no UE]" << std::endl;
                
      const uint32_t roundToSend = state.iRound;
      Simulator::Schedule(m_trainDelay, [this, socket, roundToSend]() {

        
        if (!m_roundOpen || roundToSend != m_activeRound) {
          Simulator::ScheduleNow(&Socket::Close, socket);
          return;
        }

        BuildAndSend(socket, roundToSend, m_modelPayloadBytes);
        Address loc; socket->GetSockName(loc);
        Ipv4Address ipC = InetSocketAddress::ConvertFrom(loc).GetIpv4();
        Address peerAddr; socket->GetPeerName(peerAddr);
        Ipv4Address ipS = InetSocketAddress::ConvertFrom(peerAddr).GetIpv4();

        std::cout << Simulator::Now().As(Time::S)
                  << " [R:" << (roundToSend+1) << "] [C] " << ipC
                  << " enviou w_i ("
                  << (sizeof(uint32_t)*2 + m_modelPayloadBytes)
                  << " bytes) para " << ipS << std::endl;
      });

      buf.erase(buf.begin(), buf.begin() + sizeof(uint32_t) * 2 + state.expectedSize);
      state.expectedSize = 0;
    }
  }
}

void
FlClientManager::OnNormalCloseClient(Ptr<Socket> s)
{
  Address local; s->GetSockName(local);
  Ipv4Address ip = InetSocketAddress::ConvertFrom(local).GetIpv4();
  auto it = m_clientCloseLog.find(ip);
  if (it != m_clientCloseLog.end() && it->second.closed == false) {    
    it->second.closed = true;
  }
}

void
FlClientManager::OnErrorCloseClient(Ptr<Socket> s)
{  
}

void
FlClientManager::ConnectForRound(Ipv4Address ip)
{
  auto it = m_ipToNode.find(ip);
  if (it == m_ipToNode.end()) return;
  Ptr<Node> ue = it->second;

  Ptr<Socket> cli = Socket::CreateSocket(ue, TcpSocketFactory::GetTypeId());
  cli->SetAttribute("TcpNoDelay", BooleanValue(true));
  cli->SetRecvCallback(MakeCallback(&FlClientManager::ReceivePacketClient, this));
  cli->SetCloseCallbacks(MakeCallback(&FlClientManager::OnNormalCloseClient, this),
                         MakeCallback(&FlClientManager::OnErrorCloseClient, this));

  m_clientCloseLog[ip];   
  
  Ptr<UniformRandomVariable> jitter = CreateObject<UniformRandomVariable>();
  jitter->SetAttribute("Min", DoubleValue(0.0));
  jitter->SetAttribute("Max", DoubleValue(200));

  double delay = jitter->GetValue();  
  Simulator::Schedule(
        MilliSeconds(delay),
        &Socket::Connect,
        cli,
        InetSocketAddress(m_serverIp, m_tcpPort)
  );
  
  Simulator::Schedule(Seconds(m_roundWindow.GetSeconds()), &Socket::Close, cli);
}

void
FlClientManager::ForceCloseAllClientSockets()
{ 
  for (auto &kv : m_clientBuffers) {
    Ptr<Socket> s = kv.first;
    Simulator::ScheduleNow(&Socket::Close, s);
  }
}

} // namespace ns3

