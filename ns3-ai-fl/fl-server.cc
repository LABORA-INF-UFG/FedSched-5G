#include "ns3/core-module.h"
#include "ns3/fl-global-metrics.h"
#include "fl-server.h"
#include <cstring>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(FlServer);

TypeId FlServer::GetTypeId()
{
  static TypeId tid = TypeId("ns3::FlServer")
    .SetParent<Object>()
    .SetGroupName("Applications")
    .AddConstructor<FlServer>();
  return tid;
}

FlServer::FlServer() {
  std::cout << "------------> fl-server.cc" << std::endl;
}

void
FlServer::Configure(Ptr<Node> remoteHost, Ipv4Address serverIp,
                    uint16_t tcpPort, uint32_t modelPayloadBytes,
                    Time roundWindow, uint32_t nRounds)
{
  m_remoteHost = remoteHost;
  m_serverIp = serverIp;
  m_tcpPort = tcpPort;
  m_modelPayloadBytes = modelPayloadBytes;
  m_roundWindow = roundWindow;
  m_nRounds = nRounds;
}

void
FlServer::BindAndListen()
{
  m_serverSock = Socket::CreateSocket(m_remoteHost, TcpSocketFactory::GetTypeId());
  m_serverSock->SetAttribute("TcpNoDelay", BooleanValue(true));
  m_serverSock->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_tcpPort));
  m_serverSock->Listen();
  m_serverSock->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                  MakeCallback(&FlServer::AcceptConnection, this));
}

void
FlServer::BuildAndSend(Ptr<Socket> socket, uint32_t iRound, uint32_t payloadBytes) const
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
FlServer::SendGlobalToSocket(Ptr<Socket> sock)
{
  BuildAndSend(sock, m_round, m_modelPayloadBytes);
  Address peerAddr, serverAddr; sock->GetPeerName(peerAddr); sock->GetSockName(serverAddr);
  std::cout << Simulator::Now().As(Time::S)
            << " [R:" << (m_round+1) << "] [S] "
            << InetSocketAddress::ConvertFrom(serverAddr).GetIpv4()
            << " enviou w_global (" << (sizeof(uint32_t)*2 + m_modelPayloadBytes)
            << " bytes) para " << InetSocketAddress::ConvertFrom(peerAddr).GetIpv4()
            << std::endl;
}

void
FlServer::ResetPerRoundControllers()
{
  m_serverBuffers.clear();
}

void
FlServer::CancelRoundTimer()
{
  if (m_roundEv.IsPending()) m_roundEv.Cancel();
  m_timerOpen = false;
}

uint32_t
FlServer::CountReturns() const
{
  uint32_t c = 0;
  for (const auto &kv : m_serverBuffers) if (kv.second.gotReturn) ++c;
  return c;
}

void
FlServer::AggregateAndAdvance()
{
  // Fecha TCPs que não contribuíram
  for (auto &kv : m_serverBuffers) {
    Ptr<Socket> s = kv.first;
    if (!kv.second.gotReturn) {
      Address peer; if (s->GetPeerName(peer)) {
        std::cout << Simulator::Now().As(Time::S)
                  << " [S] Encerrando TCP (não enviou no tempo) de "
                  << InetSocketAddress::ConvertFrom(peer).GetIpv4() << std::endl;
      }
      Simulator::ScheduleNow(&Socket::Close, s);
    }
  }

  // Marcacao de ponto de agregação
  const uint32_t contrib = CountReturns();      
  fl::GlobalMetrics::Instance().PushSuccessCount(contrib); 
  fl::GlobalMetrics::Instance().SetRoundEnd(Simulator::Now().GetSeconds());  
  double roundTime = fl::GlobalMetrics::Instance().GetLastRoundTime();
  std::cout << Simulator::Now().As(Time::S)
            << " [S] [RoundTime: " << roundTime << "] >>> Ponto de agregacao dos modelos locais w_i com " << contrib
            << " contribuidores na rodada " << (m_round+1) << std::endl;

  if (m_round + 1 < m_nRounds) {

    if (onRoundClosed) {
      onRoundClosed(m_round);
    }


    if (onAdvance) {
      const uint32_t next = m_round + 1;
      onAdvance(next, true);
    }
  } else {
    std::cout << Simulator::Now().As(Time::S)
              << " [S] *** FL finalizado (n_rounds atingido). ***" << std::endl;  
    
    fl::GlobalMetrics::Instance().DisableMetrics();    
    if (onAdvance) onAdvance(m_round, false);
    Simulator::Schedule(Seconds(1), &Simulator::Stop);
    
  }
  
}

void
FlServer::OnRoundTimer()
{
  if (!m_timerOpen) return;
  std::cout << Simulator::Now().As(Time::S)
            << " [S] ### TIMER da rodada " << (m_round+1) << " expirou." << std::endl;
  m_timerOpen = false;
  AggregateAndAdvance();
}

void
FlServer::AcceptConnection(Ptr<Socket> newSocket, const Address& from)
{
  Ipv4Address clientAddr = InetSocketAddress::ConvertFrom(from).GetIpv4();

  if (!m_roundStarted || !m_timerOpen) {
    std::cout << Simulator::Now().As(Time::S)
              << " [S] Recusando conexão fora da janela da rodada: " << clientAddr << std::endl;
    Simulator::ScheduleNow(&Socket::Close, newSocket);
    return;
  }

  std::cout << Simulator::Now().As(Time::S)
            << " [S] Conexão aceita de " << clientAddr << std::endl;

  m_serverBuffers[newSocket] = BufferState{};
  newSocket->SetRecvCallback(MakeCallback(&FlServer::ReceivePacketServer, this));
  newSocket->SetCloseCallbacks(MakeCallback(&FlServer::OnNormalCloseServer, this),
                               MakeCallback(&FlServer::OnErrorCloseServer, this));

  Simulator::Schedule(Seconds(m_roundWindow.GetSeconds()), &Socket::Close, newSocket);
  
  SendGlobalToSocket(newSocket);
}

void
FlServer::ReceivePacketServer(Ptr<Socket> socket)
{
  while (Ptr<Packet> packet = socket->Recv())
  {
    const uint32_t sz = packet->GetSize();

    if (sz == 0) {
      Simulator::ScheduleNow(&Socket::Close, socket);
      return;
    }

    auto it = m_serverBuffers.find(socket);
    if (it == m_serverBuffers.end()) return;
    auto &state = it->second;

    if (!m_timerOpen && !state.gotReturn) {
      std::vector<uint8_t> dummy(sz); packet->CopyData(dummy.data(), sz);
      Simulator::ScheduleNow(&Socket::Close, socket);
      return;
    }

    std::vector<uint8_t> tmp(sz);
    packet->CopyData(tmp.data(), sz);
    state.buffer.insert(state.buffer.end(), tmp.begin(), tmp.end());

    Address clienteAddr; socket->GetPeerName(clienteAddr);
    Ipv4Address ipCliente = InetSocketAddress::ConvertFrom(clienteAddr).GetIpv4();
    Address serverAddr; socket->GetSockName(serverAddr);
    Ipv4Address ipServer = InetSocketAddress::ConvertFrom(serverAddr).GetIpv4();

    while (true) {
      auto &buf = state.buffer;
      if (state.expectedSize == 0) {
        if (buf.size() < sizeof(uint32_t) * 2) break;
        std::memcpy(&state.expectedSize, buf.data(), sizeof(uint32_t));
        std::memcpy(&state.iRound, buf.data() + sizeof(uint32_t), sizeof(uint32_t));
      }
      if (buf.size() < sizeof(uint32_t) * 2 + state.expectedSize) break;

      fl::GlobalMetrics::Instance().AddSuccessElapsedTime();
      double elapsedTime = fl::GlobalMetrics::Instance().GetElapsedSinceLastStartRound();
      std::cout << Simulator::Now().As(Time::S)
                << " [R:" << (state.iRound+1) << "] [S] " << ipServer 
                << " [Tempo no Round: " << elapsedTime << "]" 
                << " recebeu w_i (" << (sizeof(uint32_t)*2 + state.expectedSize) << " bytes) de " << ipCliente
                << ". Servidor encerrando a conexao TCP." << std::endl;

      if (!state.gotReturn && onReturnSuccess) {
        onReturnSuccess(m_round, ipCliente);
      }

      buf.erase(buf.begin(), buf.begin() + sizeof(uint32_t) * 2 + state.expectedSize);
      state.expectedSize = 0;
      state.gotReturn = true;

      Simulator::ScheduleNow(&Socket::Close, socket);

      if (m_timerOpen) {
        const uint32_t returnsNow = CountReturns();
        if (returnsNow >= m_roundParticipants.size()) {
          CancelRoundTimer();
          std::cout << Simulator::Now().As(Time::S)
                    << " [S] Todos os " << returnsNow
                    << " modelos da rodada " << (m_round+1)
                    << " chegaram. Encerrando a rodada antecipadamente." << std::endl;
          AggregateAndAdvance();
          return;
        }
      }
      break;
    }
  }
}

void
FlServer::OnNormalCloseServer(Ptr<Socket> s)
{
  auto it = m_serverBuffers.find(s);
  if (it != m_serverBuffers.end()) {
    if (it->second.closedLogged) return;
    it->second.closedLogged = true;
  }
  Address a; bool ok = s->GetPeerName(a);
  if (ok) {
    // 
  } else {
    // 
  }
}

void
FlServer::OnErrorCloseServer(Ptr<Socket> s)
{
  auto it = m_serverBuffers.find(s);
  if (it != m_serverBuffers.end()) {
    if (it->second.closedLogged) return;
    it->second.closedLogged = true;
  }
  Address a; if (s->GetPeerName(a)) {
    //
  } else {
    // 
  }
}

void
FlServer::StartRound(uint32_t roundIdx, const std::vector<Ipv4Address>& participants)
{
  CancelRoundTimer();
  ResetPerRoundControllers();

  m_round = roundIdx;
  m_roundParticipants = participants;
  m_roundStarted = true;
  m_timerOpen = true;

  fl::GlobalMetrics::Instance().EnableMetrics();
  fl::GlobalMetrics::Instance().SetRoundStart(Simulator::Now().GetSeconds());

  std::cout << std::endl;

  // Tempo real
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::cout << "[Tempo real: " << std::put_time(std::localtime(&t), "%H:%M:%S") << "]"
            << std::endl;

  std::cout << Simulator::Now().As(Time::S)
            << " [S] *** Iniciando rodada " << (m_round+1)
            << " (timer=" << m_roundWindow.GetSeconds() << " s) com "
            << m_roundParticipants.size() << " UEs ***" << std::endl;

  std::cout << Simulator::Now().As(Time::S) << " [S] Participantes: ";
  for (size_t i = 0; i < m_roundParticipants.size(); ++i) {
    std::cout << m_roundParticipants[i] << (i + 1 < m_roundParticipants.size() ? ", " : "");
  }
  std::cout << std::endl;
  

  m_roundEv = Simulator::Schedule(m_roundWindow, &FlServer::OnRoundTimer, this);
}

} // namespace ns3

