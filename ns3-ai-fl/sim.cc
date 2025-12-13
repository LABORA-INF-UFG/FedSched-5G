#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/fl-global-metrics.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "ns3/nr-module.h"
#include "ns3/antenna-module.h"
#include "ns3/buildings-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-socket.h"
#include "ns3/ai-module.h"

#include <set>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>
#include <cstdlib>

#include "fl-client.h"
#include "fl-server.h"
#include "fl-gym-env.h"

using namespace ns3;

int main(int argc, char* argv[])
{
  std::cout << "------------> sim.cc" << std::endl;
 
  uint32_t ueNum;
  uint32_t uePerRound;
  uint32_t nRounds;
  double   roundWindowSec;
  double   trainDelaySec;
  std::string scheduler;
  uint32_t  enableExtraApp;
  uint32_t payloadBytes;
  uint32_t seed;
  double simTime;

  CommandLine cmd(__FILE__);
  cmd.AddValue("ueNum",       "Quantidade total de UEs no cenário", ueNum);
  cmd.AddValue("uePerRound",  "Quantidade de UEs por rodada", uePerRound);
  cmd.AddValue("n_rounds",    "Total de rodadas", nRounds);
  cmd.AddValue("roundWindow", "Janela da rodada (s)", roundWindowSec);
  cmd.AddValue("trainDelay",  "Atraso de treinamento do cliente (s)", trainDelaySec);
  cmd.AddValue("scheduler",  "Scheduler", scheduler);
  cmd.AddValue("enableExtraApp",  "Habilita extraApp", enableExtraApp);
  cmd.AddValue("payloadBytes", "payloadBytes", payloadBytes); 
  cmd.AddValue("seed",  "Seed", seed); 
  cmd.AddValue("simTime",     "Tempo total de simulação (s)", simTime);
  cmd.Parse(argc, argv);
  
  RngSeedManager::SetSeed(123);  
  RngSeedManager::SetRun(seed);
  Time::SetResolution(Time::NS);

  // TCP defaults
  Config::SetDefault("ns3::TcpSocket::RcvBufSize",   UintegerValue(1 << 24));
  Config::SetDefault("ns3::TcpSocket::SndBufSize",   UintegerValue(1 << 24));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400)); 
  Config::SetDefault ("ns3::TcpSocket::DelAckTimeout", TimeValue (MilliSeconds (0)));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName ("ns3::TcpCubic")));
  Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(1 << 24));

  std::string errorModel = "ns3::NrEesmCcT1";
  Config::SetDefault("ns3::NrAmc::ErrorModelType", TypeIdValue(TypeId::LookupByName(errorModel)));
  Config::SetDefault("ns3::NrAmc::AmcModel", EnumValue(NrAmc::ShannonModel)); 
  
  NodeContainer gnbNode; gnbNode.Create(1);
  NodeContainer ueNodes; ueNodes.Create(ueNum);
  NodeContainer extraUesUL; extraUesUL.Create(uePerRound); 
    
  RngSeedManager::SetRun(321);
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();
  pos->Add(Vector(0.0, 0.0, 50.0)); // gNB  

  Ptr<UniformRandomVariable> r  = CreateObject<UniformRandomVariable>(); 
  r->SetAttribute("Min", DoubleValue(20.0));
  r->SetAttribute("Max", DoubleValue(160.0));
  Ptr<UniformRandomVariable> th = CreateObject<UniformRandomVariable>();
  th->SetAttribute("Min", DoubleValue(0.0));
  th->SetAttribute("Max", DoubleValue(2 * M_PI));
  for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
    double Rv = r->GetValue();
    double T  = th->GetValue();
    pos->Add(Vector(Rv * std::cos(T), Rv * std::sin(T), 1.5));
  }

  Ptr<UniformRandomVariable> r2  = CreateObject<UniformRandomVariable>();
  r2->SetAttribute("Min", DoubleValue(20.0));
  r2->SetAttribute("Max", DoubleValue(80.0));
  Ptr<UniformRandomVariable> th2 = CreateObject<UniformRandomVariable>();
  th2->SetAttribute("Min", DoubleValue(0.0));
  th2->SetAttribute("Max", DoubleValue(2 * M_PI));
  for (uint32_t i = 0; i < extraUesUL.GetN(); ++i) {
    double Rv = r2->GetValue();
    double T  = th2->GetValue();
    pos->Add(Vector(Rv * std::cos(T), Rv * std::sin(T), 1.5));
  }

  mobility.SetPositionAllocator(pos);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(gnbNode);      
  mobility.Install(ueNodes);      
  mobility.Install(extraUesUL);  
  RngSeedManager::SetRun(seed);

  // Helpers NR/EPC
  Ptr<NrPointToPointEpcHelper> nrEpcHelper = CreateObject<NrPointToPointEpcHelper>();
  Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
  
  std::map<std::string, std::string> schedulerTypeId;
  schedulerTypeId["FL"] = "ns3::NrMacSchedulerOfdmaFL";
  schedulerTypeId["RR"] = "ns3::NrMacSchedulerOfdmaRR";
  schedulerTypeId["MR"] = "ns3::NrMacSchedulerOfdmaMR";
  schedulerTypeId["PF"] = "ns3::NrMacSchedulerOfdmaPF";
  nrHelper->SetSchedulerTypeId(TypeId::LookupByName(schedulerTypeId[scheduler]));
  fl::GlobalMetrics::Instance().SetSchedulerCode(scheduler);
  fl::GlobalMetrics::Instance().SetSeed(seed);
   
  Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper>();
  nrHelper->SetBeamformingHelper(idealBeamformingHelper);
  nrHelper->SetEpcHelper(nrEpcHelper);
  
  channelHelper->ConfigureFactories("UMa", "Default", "ThreeGpp");  

  uint32_t numerology = 1;
  BandwidthPartInfoPtrVector allBwps;
  CcBwpCreator ccBwpCreator; double f0 = 4e9, bw = 10e6; uint8_t numCcPerBand = 1; 
  CcBwpCreator::SimpleOperationBandConf bandConf(f0, bw, numCcPerBand);  
  OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);
 
  channelHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false)); 
  Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(150)));  
  channelHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(0))); 
 
  channelHelper->AssignChannelsToBands({band});
  allBwps = CcBwpCreator::GetAllBwps({band});

  idealBeamformingHelper->SetAttribute("BeamformingMethod", TypeIdValue(DirectPathBeamforming::GetTypeId()));
  nrEpcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));

  nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
  nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));
  nrHelper->SetUeAntennaAttribute("AntennaElement",
                                  PointerValue(CreateObject<IsotropicAntennaModel>()));

  nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4));
  nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
  nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

  nrHelper->SetUlErrorModel(errorModel);
  nrHelper->SetDlErrorModel(errorModel);

  nrHelper->SetGnbDlAmcAttribute("AmcModel", EnumValue(NrAmc::ShannonModel));
  nrHelper->SetGnbUlAmcAttribute("AmcModel", EnumValue(NrAmc::ShannonModel));
  
  NetDeviceContainer gnbDev = nrHelper->InstallGnbDevice(gnbNode, allBwps);
  NetDeviceContainer ueDevs = nrHelper->InstallUeDevice(ueNodes, allBwps);
  NetDeviceContainer extraUesULDev = nrHelper->InstallUeDevice(extraUesUL, allBwps);

  double gnbTxPower = 41;
  double ueTxPower  = 23;

  Ptr<NrGnbNetDevice> gnb = DynamicCast<NrGnbNetDevice>(gnbDev.Get(0));
  Ptr<NrGnbPhy> gnbPhy = gnb->GetPhy(0);
  gnbPhy->SetAttribute("TxPower", DoubleValue(gnbTxPower));
  gnbPhy->SetNumerology(numerology);
  
  for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
    Ptr<NrUePhy> uePhy = nrHelper->GetUePhy(ueDevs.Get(i), 0);
    uePhy->SetAttribute("TxPower", DoubleValue(ueTxPower));
    uePhy->SetNumerology(numerology);
  }

  for (uint32_t i = 0; i < extraUesUL.GetN(); ++i) {
    Ptr<NrUePhy> uePhy = nrHelper->GetUePhy(extraUesULDev.Get(i), 0);
   uePhy->SetAttribute("TxPower", DoubleValue(ueTxPower));
   uePhy->SetNumerology(numerology);
  }
  
  DoubleValue cfgTx;
  gnbPhy->GetAttribute("TxPower", cfgTx);
  double normTx = gnbPhy->GetTxPower();

  UintegerValue cfgNumerology;
  gnbPhy->GetAttribute("Numerology", cfgNumerology);
  uint32_t cfgNumerologyVal = cfgNumerology.Get();

  std::cout << "Seed: " << seed << " | gNB "
            << " -> Configured TxPower = " << cfgTx.Get()
            << " dBm, Normalized = " << normTx << " dBm, " 
            << " Numerology: " << cfgNumerologyVal << std::endl;

  for (uint32_t i = 0; i < ueDevs.GetN(); ++i) {
    Ptr<NrUeNetDevice> ue = DynamicCast<NrUeNetDevice>(ueDevs.Get(i));
    Ptr<NrUePhy> uePhy = ue->GetPhy(0);
    DoubleValue cfgTxUe;
    uePhy->GetAttribute("TxPower", cfgTxUe);
    double normTxUe = uePhy->GetTxPower();
    
    uint8_t muUe = uePhy->GetNumerology();
    std::cout << "UE " << i
              << " -> Configured TxPower = " << cfgTxUe.Get() 
              << " dBm, Normalized = " << normTxUe << " dBm, "
              << " Numerology: " << unsigned(muUe) << std::endl;
    break;
  }

  std::cout << "enableExtraApp: " << enableExtraApp << std::endl;

  InternetStackHelper internet; 
  internet.Install(ueNodes);
  internet.Install(extraUesUL);

  auto rhSetup = nrEpcHelper->SetupRemoteHost("100Gb/s", 2500, Seconds(0.000));
  Ptr<Node> remoteHost = rhSetup.first;
  Ipv4Address serverIP = remoteHost->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

  nrEpcHelper->AssignUeIpv4Address(ueDevs);
  Ipv4StaticRoutingHelper ipv4;
  for (uint32_t u = 0; u < ueNodes.GetN(); ++u) {
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4.GetStaticRouting(ueNodes.Get(u)->GetObject<Ipv4>());
    ueStaticRouting->SetDefaultRoute(nrEpcHelper->GetUeDefaultGatewayAddress(), 1);
  }

  Ptr<Node> remoteHost2 = CreateObject<Node>();;
  NodeContainer remoteHost2Container;
  remoteHost2Container.Add(remoteHost2);
  internet.Install(remoteHost2Container);

  Ptr<Node> pgw = nrEpcHelper->GetPgwNode();
  internet.Install(pgw);

  NodeContainer p2pNodes(pgw, remoteHost2);
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("100Gb/s"));
  p2p.SetChannelAttribute("Delay", TimeValue(Seconds(0.0)));
  NetDeviceContainer p2pDevices = p2p.Install(p2pNodes);

  Ipv4AddressHelper ipv4rh2;
  ipv4rh2.SetBase("5.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer if2 = ipv4rh2.Assign(p2pDevices);

  Ptr<Ipv4> ipv4_rh2 = remoteHost2->GetObject<Ipv4>();
  Ptr<Ipv4StaticRouting> staticRh2 =
      Ipv4StaticRoutingHelper().GetStaticRouting(ipv4_rh2);
  staticRh2->AddNetworkRouteTo(
      Ipv4Address("7.0.0.0"),     
      Ipv4Mask("255.0.0.0"),      
      if2.GetAddress(0),          
      1                           
  );   
  nrEpcHelper->AssignUeIpv4Address(NetDeviceContainer(extraUesULDev));

  nrHelper->AttachToClosestGnb(ueDevs, gnbDev);
  nrHelper->AttachToClosestGnb(extraUesULDev, gnbDev);

  std::map<Ipv4Address, uint32_t, FlClientManager::Ipv4AddrLess> ip2id;
  std::vector<Ipv4Address> allIpsById;
  allIpsById.resize(ueNodes.GetN());
  for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
    Ptr<Ipv4> ipv4L3 = ueNodes.Get(i)->GetObject<Ipv4>();
    Ipv4Address ip = ipv4L3->GetAddress(1, 0).GetLocal();
    ip2id[ip] = i + 1;          
    allIpsById[i] = ip;
  }

  Ptr<FlClientManager> clientMgr = CreateObject<FlClientManager>();
  clientMgr->Configure(serverIP, /*TcpPort*/ 9, payloadBytes, Seconds(roundWindowSec), Seconds(trainDelaySec));
  for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
    clientMgr->RegisterNode(allIpsById[i], ueNodes.Get(i));
  }

  Ptr<FlServer> server = CreateObject<FlServer>();
  server->Configure(remoteHost, serverIP, /*TcpPort*/ 9, payloadBytes,
                    Seconds(roundWindowSec), nRounds);
  server->BindAndListen();


  Simulator::Schedule (MilliSeconds (250), [gnbNode, ueNodes, ueDevs, extraUesUL, extraUesULDev] () {
    Ptr<MobilityModel> gnbMob = gnbNode.Get(0)->GetObject<MobilityModel>();
    Vector gnbPos = gnbMob->GetPosition();  
    for (uint32_t i = 0; i < ueNodes.GetN(); ++i)
    {
        Ptr<MobilityModel> ueMob = ueNodes.Get(i)->GetObject<MobilityModel>();
        Vector uePos = ueMob->GetPosition();
        double dist = CalculateDistance(gnbPos, uePos);
        Ptr<NrUeNetDevice> ueNrDev = DynamicCast<NrUeNetDevice>(ueDevs.Get(i));        
        Ptr<NrUeRrc> rrc = ueNrDev->GetRrc();
        uint16_t rnti = rrc->GetRnti(); 
        fl::GlobalMetrics::Instance().AddUE(i, (i+1), rnti, dist);
    }   
    
    if (!fl::GlobalMetrics::Instance().HasSameRntiLoadValues()) { 
      std::cout << "\nRNITs incompativeis com vetor pre-carregado!" << std::endl; 
      auto rntiVec = fl::GlobalMetrics::Instance().GetRntiVectorFromUeMap();
      for (auto rnti : rntiVec) {
        std::cout << rnti << ", ";
      }
      std::cout << std::endl;        
      exit(1);      
    }

    for (uint32_t i = 0; i < extraUesUL.GetN(); ++i)
    {
        Ptr<MobilityModel> ueMob = extraUesUL.Get(i)->GetObject<MobilityModel>();
        Vector uePos = ueMob->GetPosition();
        double dist = CalculateDistance(gnbPos, uePos);
        Ptr<NrUeNetDevice> ueNrDev = DynamicCast<NrUeNetDevice>(extraUesULDev.Get(i));        
        Ptr<NrUeRrc> rrc = ueNrDev->GetRrc();
        
        uint16_t rnti = rrc->GetRnti(); 
        fl::GlobalMetrics::Instance().AddExtraUE(i, (i+1), rnti, dist);
    }   
  });

  if (enableExtraApp) { 


    uint16_t port = 50000;
    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(remoteHost2);
    sinkApp.Start(Seconds(1));
    sinkApp.Stop(Seconds(simTime));

    OnOffHelper onoff("ns3::UdpSocketFactory",
                      InetSocketAddress(ipv4_rh2->GetAddress(1, 0).GetLocal(), port));
   
    Ptr<UniformRandomVariable> randRate = CreateObject<UniformRandomVariable>();
    
    randRate->SetAttribute("Min", DoubleValue(1e6));   
    randRate->SetAttribute("Max", DoubleValue(1.5e6));   
    double rate = randRate->GetValue();
    std::ostringstream rateStr;
    rateStr << static_cast<uint64_t>(rate) << "bps";  
    onoff.SetAttribute("DataRate", StringValue(rateStr.str()));     
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    
    const double dMin    = 0.95;   
    const double dMax    = 0.1;  
    const double OffMean = 0.010;  // 10 ms  

    const double OnMin = OffMean * (dMin / (1.0 - dMin));
    const double OnMax = OffMean * (dMax / (1.0 - dMax));

    Ptr<UniformRandomVariable> onU = CreateObject<UniformRandomVariable>();
    onU->SetAttribute("Min", DoubleValue(OnMin));
    onU->SetAttribute("Max", DoubleValue(OnMax));
   
    std::ostringstream offStr;
    offStr << "ns3::ExponentialRandomVariable[Mean=" << OffMean << "]";
    const std::string offStrVal = offStr.str();    

    for (uint32_t i = 0; i < extraUesUL.GetN(); ++i)
    {
      const double onMean = onU->GetValue();
      std::ostringstream onStr;
      onStr << "ns3::ExponentialRandomVariable[Mean=" << onMean << "]";
  
      onoff.SetAttribute("OnTime",  StringValue(onStr.str()));
      onoff.SetAttribute("OffTime", StringValue(offStrVal));

      ApplicationContainer app = onoff.Install(extraUesUL.Get(i));
      app.Start(Seconds(1.0));
      app.Stop(Seconds(simTime));
    }
  }

  // Gym Env
  Ptr<FlGymEnv> env = CreateObject<FlGymEnv>();
  env->SetServer(server);
  env->SetClientManager(clientMgr);
  env->SetIpToNode(ip2id);
  env->SetUeNum(ueNum);
  env->SetKPerRound(uePerRound);
  env->SetNumRounds(nRounds);
  env->SetRoundWindow(Seconds(roundWindowSec));

  // Calcula SINR
  auto getPos = [](Ptr<Node> n) {
    return n->GetObject<MobilityModel>()->GetPosition();
  };
  Vector gnbPos = getPos(gnbNode.Get(0));
  const double c = 299792458.0;
  const double lambda = c / f0; 
  const double noise_dBm = -174.0 + 10.0*std::log10(bw) + 7.0; 
  std::vector<double> sinrDb(ueNum, 0.0);
  for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
    Vector up = getPos(ueNodes.Get(i));
    double dx = up.x - gnbPos.x;
    double dy = up.y - gnbPos.y;
    double dz = up.z - gnbPos.z;
    double d = std::sqrt(dx*dx + dy*dy + dz*dz);    
    if (d < 1.0) d = 1.0; 
    double fspl_dB = 20.0*std::log10(4.0*M_PI*d/lambda);
    double rx_dBm = ueTxPower - fspl_dB;
    double snr_dB = rx_dBm - noise_dBm;
    sinrDb[i] = snr_dB;
  }
  
  env->SetSinrVector(sinrDb);  
  Simulator::Schedule(MilliSeconds(200), &FlGymEnv::Notify, env);
  
  FlowMonitorHelper flowmonHelper;
  NodeContainer endpointNodes;
  endpointNodes.Add(remoteHost);   
  endpointNodes.Add(ueNodes);

  std::map<Ipv4Address,int> dstList = {
    { serverIP, 0 }
  };

  if (enableExtraApp) {
    endpointNodes.Add(remoteHost2);   
    endpointNodes.Add(extraUesUL);
    dstList.insert({ipv4_rh2->GetAddress(1, 0).GetLocal(), 1 });
  }

  Ptr<FlowMonitor> flowmon = flowmonHelper.Install(endpointNodes);

  Simulator::Stop(Seconds(simTime)); 
  Simulator::Run();  

  flowmon->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
  auto stats = flowmon->GetFlowStats();
  
  std::cout << "fl::GlobalMetrics::Instance().DumpCsv()" << std::endl;
  fl::GlobalMetrics::Instance().DumpCsv();
  fl::GlobalMetrics::Instance().DumpFlowStatsCsv(stats, classifier, serverIP);
  fl::GlobalMetrics::Instance().DumpFlowStatsPerRoundCsv(stats, classifier, dstList);

  OpenGymInterface::Get()->NotifySimulationEnd();

  Simulator::Destroy();
  return 0;
}
