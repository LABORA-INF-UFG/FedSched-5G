#pragma once
#include "ns3/flow-monitor-module.h"
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <cstdint>
#include <filesystem>
#include <iomanip>

namespace ns3::fl {

class GlobalMetrics
{
public:
  static GlobalMetrics& Instance()
  {    
    static GlobalMetrics inst;
    return inst;
  }
  
  uint32_t GetRound() const { return m_round; }
  void     SetRound(uint32_t r) { m_round = r; }
  void     IncRound() { ++m_round; }

  void     SetSeed(uint32_t seed) { m_seed = seed; }

  void EnableMetrics()  { m_collectMetrics = true;  }
  void DisableMetrics() { m_collectMetrics = false; }

  void PushSuccessCount(uint32_t successCount)
  {
    m_successPerRound.push_back(successCount);
  }

  void PushRnitUELoad(uint32_t rnit)
  {
    m_rnitUELoad.push_back(rnit);
  }
  
  bool HasSameRntiLoadValues() const
  {   
    std::set<uint32_t> rntiFromMap;

    for (const auto& [nodeId, tup] : m_ueMap)
    {
      uint16_t rnti = std::get<1>(tup);  
      rntiFromMap.insert(rnti);
    }    
    std::set<uint32_t> rntiFromVec(m_rnitUELoad.begin(), m_rnitUELoad.end()); 

    return rntiFromVec == rntiFromMap;
  }

  std::vector<uint32_t> GetRntiVectorFromUeMap() const
  {
    std::vector<uint32_t> rntis;
    rntis.reserve(m_ueMap.size()); 

    for (const auto& [nodeId, tup] : m_ueMap)
    {
      uint16_t rnti = std::get<1>(tup);  
      rntis.push_back(rnti);
    }

    return rntis;
  }
  
  void AddUlRbgUeFL(uint32_t rbgCount)
  {
    if (m_collectMetrics) {
      if (m_ulRbgUeFLPerRound.find(m_round) == m_ulRbgUeFLPerRound.end())
      {
        m_ulRbgUeFLPerRound[m_round] = 0;
      }
      m_ulRbgUeFLPerRound[m_round] += rbgCount;
    }
  }
  
  void AddUlMcsUeFL(uint32_t mcsValue)
  {
    if (m_collectMetrics) {
      if (m_ulMCSUeFLPerRound.find(m_round) == m_ulMCSUeFLPerRound.end())
      {
        m_ulMCSUeFLPerRound[m_round] = {0, 0};  
      }
      m_ulMCSUeFLPerRound[m_round].first  += mcsValue; 
      m_ulMCSUeFLPerRound[m_round].second += 1;       
    }
  } 
  
  void AddUlNumSymUeFL(uint32_t numSym)
  {
    if (m_collectMetrics) {
      if (m_ulNumSymUeFLPerRound.find(m_round) == m_ulNumSymUeFLPerRound.end())
      {
        m_ulNumSymUeFLPerRound[m_round] = 0;
      }
      m_ulNumSymUeFLPerRound[m_round] += numSym;
    }
  }
  
  void AddUlTbSizeUeFL(uint32_t tbSize)
  {
    if (m_collectMetrics) {
      if (m_ulTbSizeUeFLPerRound.find(m_round) == m_ulTbSizeUeFLPerRound.end())
      {
        m_ulTbSizeUeFLPerRound[m_round] = 0;
      }
      m_ulTbSizeUeFLPerRound[m_round] += tbSize;
    }
  }

  void AddUlNdiUeFL(uint32_t ndi)
  {
    if (m_collectMetrics)
    {      
      auto& nd = m_ulNdiUeFLPerRound[m_round];
      if (ndi == 0) {
        ++nd.first;   
      } else if (ndi == 1) {
        ++nd.second;  
      } 
    }
  } 
  
  void AddUlRbgUeNFL(uint32_t rbgCount)
  {
    if (m_collectMetrics) {
      if (m_ulRbgUeNFLPerRound.find(m_round) == m_ulRbgUeNFLPerRound.end())
      {
        m_ulRbgUeNFLPerRound[m_round] = 0;
      }
      m_ulRbgUeNFLPerRound[m_round] += rbgCount;
    }
  }

  void IncSchedCfgCntUeFL()
  {
    if (m_collectMetrics)
    {
      if (m_schedCfgCntUeFLPerRound.find(m_round) == m_schedCfgCntUeFLPerRound.end())
      {
        m_schedCfgCntUeFLPerRound[m_round] = 0;
      }
      m_schedCfgCntUeFLPerRound[m_round] += 1;
    }
  }
  
  void AddSuccessId(uint32_t id)
  {
    if (m_successIdsPerRound.find(m_round) == m_successIdsPerRound.end())
    {
      m_successIdsPerRound[m_round] = std::vector<uint32_t>();
    }
    m_successIdsPerRound[m_round].push_back(id);
  }  
  
  void SetRoundStart(double time)
  {
    m_roundTimes[m_round].first = time;
    auto now = std::chrono::system_clock::now();
    double wallSec = std::chrono::duration_cast<std::chrono::duration<double>>(
                      now.time_since_epoch()).count();
    m_roundWall[m_round].first = wallSec;
  }
  void SetRoundEnd(double time)
  {
    m_roundTimes[m_round].second = time;
    auto now = std::chrono::system_clock::now();
    double wallSec = std::chrono::duration_cast<std::chrono::duration<double>>(
                      now.time_since_epoch()).count();
    m_roundWall[m_round].second = wallSec;
  }
  const std::map<uint32_t, std::pair<double,double>>& GetRoundTimes() const 
  { 
    return m_roundTimes; 
  }

  double GetLastRoundTime() const
  {
    if (m_roundTimes.empty()) {         
        return -1.0;
    }          
    const auto& lastPair = std::cref(m_roundTimes.rbegin()->second).get();
    const double start = lastPair.first;  
    const double end   = lastPair.second; 
    return end - start;
  }

  double GetElapsedSinceLastStartRound() const
  {
      if (m_roundTimes.empty()) {
          return -1.0;
      }
      
      const auto& last = m_roundTimes.rbegin()->second;
      const double start = last.first;               
      const double now   = ns3::Simulator::Now().GetSeconds(); 
      return now - start; 
  }

  void AddSuccessElapsedTime()
  {
      if (m_successElapsedTimePerRound.find(m_round) == m_successElapsedTimePerRound.end())
      {
        m_successElapsedTimePerRound[m_round] = std::vector<double>();
      }
      m_successElapsedTimePerRound[m_round].push_back(GetElapsedSinceLastStartRound());
  }
 
  void DumpCsv(const std::string& base = "")
  {    
    const std::string outDir = m_outDir + "/seed" + std::to_string(m_seed) + "/" + m_schedulerCode + "/";
    std::filesystem::create_directories(outDir);
    
    {
       std::ofstream o(outDir + base + "roundTimes.csv");
        o << "round,start_sim,end_sim,duration_sim,start_wall,end_wall,duration_wall\n";
        
        auto ToString = [](double wallSec) -> std::string {
          if (wallSec < 0.0) return "";
          std::time_t t = static_cast<std::time_t>(wallSec);
          std::tm tm = *std::localtime(&t);
          std::ostringstream oss;
          oss << std::put_time(&tm, "%H:%M:%S");
          return oss.str();
        };

        for (auto const& [round, times] : m_roundTimes)
        {
          double durationSim = times.second - times.first;
          
          auto wallIt = m_roundWall.find(round);
          double startWall = wallIt != m_roundWall.end() ? wallIt->second.first  : -1.0;
          double endWall   = wallIt != m_roundWall.end() ? wallIt->second.second : -1.0;
          double durationWall = (startWall >= 0.0 && endWall >= 0.0) ? (endWall - startWall) : -1.0;
          
          o << round << ","
            << times.first << "," << times.second << "," << durationSim << ","
            << ToString(startWall) << "," << ToString(endWall) << "," << durationWall
            << "\n";
        }
    }
    
    {
      std::ofstream o(outDir + base + "success.csv");
      o << "round,success_count\n";
      for (size_t i = 0; i < m_successPerRound.size(); ++i)
      {
        o << (i + 1) << "," << m_successPerRound[i] << "\n";
      }
    }    
   
    {
      std::ofstream o(outDir + base + "ulRbgUeFL.csv");
      o << "round,total_ul_rbg\n";
      for (auto const& [round, total] : m_ulRbgUeFLPerRound)
      {
        o << round << "," << total << "\n";
      }
    }
      
    {
      std::ofstream o(outDir + base + "ulRbgUeNFL.csv");
      o << "round,total_ul_rbg\n";
      for (auto const& [round, total] : m_ulRbgUeNFLPerRound)
      {
        o << round << "," << total << "\n";
      }
    }
    
    {
      std::ofstream o(outDir + base + "ulMcsUeFL.csv");
      o << "round,avg_ul_mcs\n";
      for (auto const& [round, data] : m_ulMCSUeFLPerRound)
      {
        uint64_t soma   = data.first;
        uint64_t count  = data.second;
        double media    = (count > 0) ? (double)soma / (double)count : 0.0;

        o << round << "," << media << "\n";
      }
    }
    
    {
      std::ofstream o(outDir + base + "ulNumSymUeFL.csv");
      o << "round,total_numsym\n";
      for (auto const& [round, total] : m_ulNumSymUeFLPerRound)
      {
        o << round << "," << total << "\n";
      }
    }    
    
    {
      std::ofstream o(outDir + base + "ulTbSizeUeFL.csv");
      o << "round,total_tbSize\n";
      for (auto const& [round, total] : m_ulTbSizeUeFLPerRound)
      {
        o << round << "," << total << "\n";
      }
    } 
    
    {
      std::ofstream o(outDir + base + "ulNdiUeFL.csv");
      o << "round,ndi0_count,ndi1_count\n";
      for (const auto& [round, counts] : m_ulNdiUeFLPerRound)
      {
        o << round << "," << counts.first << "," << counts.second << "\n";
      }
    }

    {
      std::ofstream o(outDir + base + "schedCfgCntUeFLPerRound.csv");
      o << "round,count\n";
      for (const auto& [round, cnt] : m_schedCfgCntUeFLPerRound)
      {
        o << round << "," << cnt << "\n";
      }
    }
    
    {
      std::ofstream o(outDir + base + "successIds.csv");
      o << "round,ids\n";
      for (auto const& [round, ids] : m_successIdsPerRound)
      {
        o << round << ",";
        for (size_t i = 0; i < ids.size(); ++i)
        {
          o << ids[i];
          if (i < ids.size() - 1) o << ";";
        }
        o << "\n";
      }
    }
     
    {
      std::ofstream o(outDir + base + "successElapsedTime.csv");
      o << "round,ids\n";
      for (auto const& [round, elapsedTime] : m_successElapsedTimePerRound)
      {
        o << round << ",";
        for (size_t i = 0; i < elapsedTime.size(); ++i)
        {
          o << elapsedTime[i];
          if (i < elapsedTime.size() - 1) o << ";";
        }
        o << "\n";
      }
    }    
   
  {
    std::ofstream o(outDir + base + "ues.csv");
    o << "node_id,ue_idx,rnti,distance_m\n";
    o << std::fixed << std::setprecision(6);
    for (const auto& kv : m_ueMap)
    {
      uint32_t nodeId = kv.first;
      uint32_t ueIdx;
      uint16_t rnti;
      double dist;
      std::tie(ueIdx, rnti, dist) = kv.second;

      o << nodeId << "," << ueIdx << "," << rnti << "," << dist << "\n";
    }
  }
   
  {
    std::ofstream o(outDir + base + "uesExtra.csv");
    o << "node_id,ue_idx,rnti,distance_m\n";
    o << std::fixed << std::setprecision(6);
    for (const auto& kv : m_extaUeMap)
    {
      uint32_t nodeId = kv.first;
      uint32_t ueIdx;
      uint16_t rnti;
      double dist;
      std::tie(ueIdx, rnti, dist) = kv.second;

      o << nodeId << "," << ueIdx << "," << rnti << "," << dist << "\n";
    }
  }

  }

void DumpFlowStatsCsv(
    const std::map<FlowId, FlowMonitor::FlowStats>& stats,
    Ptr<Ipv4FlowClassifier> classifier,
    const Ipv4Address& serverIP,
    const std::string& base = "")
{
  const std::string outDir = m_outDir + "/seed" + std::to_string(m_seed) + "/" + m_schedulerCode + "/";
  std::filesystem::create_directories(outDir);

  std::ofstream o(outDir + base + "flowStats.csv");

  o << "flow_id,src,sport,dst,dport,start,end,active_s,"
       "tx_kbit_s,rx_kbit_s,throughput_kbit_s,"
       "mean_delay_ms,mean_jitter_ms,packet_loss_pct,delivery_ratio_pct,"
       "tx_pkts,rx_pkts,tx_bytes,rx_bytes\n";

  for (const auto& entry : stats)
  {
    const FlowId flowId = entry.first;
    const FlowMonitor::FlowStats& fs = entry.second;

    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flowId);

    if (t.sourceAddress == serverIP)
      continue;

    const double startTime      = fs.timeFirstTxPacket.GetSeconds();
    const double endTime        = fs.timeLastRxPacket.GetSeconds();
    const double activeDuration = (endTime > startTime) ? (endTime - startTime) : 0.0;

    const double txKbitPerSec =
        (activeDuration > 0.0) ? ((fs.txBytes * 8.0 / 1000.0) / activeDuration) : 0.0;
    const double rxKbitPerSec =
        (activeDuration > 0.0) ? ((fs.rxBytes * 8.0 / 1000.0) / activeDuration) : 0.0;
    const double throughputKbitPerSec = rxKbitPerSec;

    const double meanDelayMs =
        (fs.rxPackets > 0) ? (fs.delaySum.GetSeconds() * 1000.0 / fs.rxPackets) : 0.0;

    const double lossRatioPct =
        (fs.txPackets > 0) ? (100.0 * (fs.txPackets - fs.rxPackets) / fs.txPackets) : 0.0;

    const double meanJitterMs =
        (fs.rxPackets > 1) ? (fs.jitterSum.GetSeconds() * 1000.0 / (fs.rxPackets - 1)) : 0.0;

    const double deliveryRatioPct =
        (fs.txPackets > 0) ? (100.0 * (double)fs.rxPackets / (double)fs.txPackets) : 0.0;

    o << flowId << ","
      << t.sourceAddress << "," << t.sourcePort << ","
      << t.destinationAddress << "," << t.destinationPort << ","
      << startTime << "," << endTime << "," << activeDuration << ","
      << txKbitPerSec << "," << rxKbitPerSec << "," << throughputKbitPerSec << ","
      << meanDelayMs << "," << meanJitterMs << "," << lossRatioPct << "," << deliveryRatioPct << ","
      << fs.txPackets << "," << fs.rxPackets << ","
      << fs.txBytes << "," << fs.rxBytes
      << "\n";
  }
}

void DumpFlowStatsPerRoundCsv(
    const std::map<FlowId, FlowMonitor::FlowStats>& stats,
    Ptr<Ipv4FlowClassifier> classifier,
    const std::map<Ipv4Address,int>& dstList,
    const std::string& base = "")
{
  const std::string outDir = m_outDir + "/seed" + std::to_string(m_seed) + "/" + m_schedulerCode + "/";
  std::cout << outDir << std::endl;
  std::filesystem::create_directories(outDir);

  std::ofstream o(outDir + base + "flowStatsPerRound.csv");
  o << "round,start,end,dur_s,dest,"
       "tx_kbit_s,rx_kbit_s,throughput_kbit_s,"
       "mean_delay_ms,mean_jitter_ms,packet_loss_pct,delivery_ratio_pct,"
       "tx_pkts,rx_pkts,tx_bytes,rx_bytes\n";

  const size_t D = dstList.size();
  std::vector<Ipv4Address> idx2dst(D);
  for (auto const& [addr, idx] : dstList)
    if (idx < static_cast<int>(D)) idx2dst[idx] = addr;

  for (auto const& [round, times] : GetRoundTimes())
  {
    const double winStart = times.first;
    const double winEnd   = times.second;
    const double winDur   = std::max(0.0, winEnd - winStart);
    if (winDur <= 0.0) continue;
    
    std::vector<double>   sumTxBytes_r(D, 0.0);
    std::vector<double>   sumRxBytes_r(D, 0.0);
    std::vector<double>   sumDelay_r_s(D, 0.0);
    std::vector<double>   sumJitter_r_s(D, 0.0);
    std::vector<uint64_t> sumTxPkts_r(D, 0ull);
    std::vector<uint64_t> sumRxPkts_r(D, 0ull);
    std::vector<uint32_t> flowsWithRxInWin(D, 0u);

    for (const auto& entry : stats)
    {
      const FlowId fid = entry.first;
      const FlowMonitor::FlowStats& fs = entry.second;
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(fid);

      auto itDst = dstList.find(t.destinationAddress);
      if (itDst == dstList.end()) continue;
      const int di = itDst->second;

      const double fStart  = fs.timeFirstTxPacket.GetSeconds();
      const double fEnd    = fs.timeLastRxPacket.GetSeconds();
      const double fActive = std::max(0.0, fEnd - fStart);
      if (fActive <= 0.0) continue;

      const double L = std::max(winStart, fStart);
      const double R = std::min(winEnd,   fEnd);
      const double overlap = std::max(0.0, R - L);
      if (overlap <= 0.0) continue;

      const double frac = overlap / fActive;
      
      sumTxBytes_r[di] += fs.txBytes   * frac;
      sumRxBytes_r[di] += fs.rxBytes   * frac;
      sumDelay_r_s[di] += fs.delaySum.GetSeconds()  * frac;
      sumJitter_r_s[di]+= fs.jitterSum.GetSeconds() * frac;
      
      const uint64_t txPkts_r = (uint64_t) llround(fs.txPackets * frac);
      const uint64_t rxPkts_r = (uint64_t) llround(fs.rxPackets * frac);
      sumTxPkts_r[di] += txPkts_r;
      sumRxPkts_r[di] += rxPkts_r;
      if (rxPkts_r >= 1) ++flowsWithRxInWin[di];
    }

    for (size_t di = 0; di < D; ++di)
    {
      std::ostringstream oss; oss << idx2dst[di];
      const std::string dest = oss.str();
      
      const double txKbitPerSec =
          (winDur > 0.0) ? ((sumTxBytes_r[di] * 8.0 / 1000.0) / winDur) : 0.0;
      const double rxKbitPerSec =
          (winDur > 0.0) ? ((sumRxBytes_r[di] * 8.0 / 1000.0) / winDur) : 0.0;
      const double throughputKbitPerSec = rxKbitPerSec;

      const double meanDelayMs =
          (sumRxPkts_r[di] > 0)
            ? (sumDelay_r_s[di] * 1000.0 / (double)sumRxPkts_r[di])
            : 0.0;

      const double denomJitterPkts =
          (double)sumRxPkts_r[di] - (double)flowsWithRxInWin[di];
      const double meanJitterMs =
          (denomJitterPkts > 0.0)
            ? (sumJitter_r_s[di] * 1000.0 / denomJitterPkts)
            : 0.0;

      const double lossRatioPct =
          (sumTxPkts_r[di] > 0)
            ? (100.0 * ((double)sumTxPkts_r[di] - (double)sumRxPkts_r[di]) / (double)sumTxPkts_r[di])
            : 0.0;

      const double deliveryRatioPct =
          (sumTxPkts_r[di] > 0)
            ? (100.0 * (double)sumRxPkts_r[di] / (double)sumTxPkts_r[di])
            : 0.0;
      
      const uint64_t txBytesInt = (uint64_t) llround(sumTxBytes_r[di]);
      const uint64_t rxBytesInt = (uint64_t) llround(sumRxBytes_r[di]);

      o << round << ","
        << winStart << "," << winEnd << "," << winDur << ","
        << dest << ","
        << txKbitPerSec << "," << rxKbitPerSec << "," << throughputKbitPerSec << ","
        << meanDelayMs << "," << meanJitterMs << "," << lossRatioPct << "," << deliveryRatioPct << ","
        << sumTxPkts_r[di] << "," << sumRxPkts_r[di] << ","
        << txBytesInt << "," << rxBytesInt
        << "\n";
    }
  }
}

void AddUE(uint32_t nodeId, uint32_t ueIdx, uint16_t rnti, double distanceM)
{
  m_ueMap[nodeId] = std::make_tuple(ueIdx, rnti, distanceM);
}

bool HasRntiUE(uint16_t rnti) const
{
  for (const auto& kv : m_ueMap)
  {
    uint32_t ueIdx;
    uint16_t storedRnti;
    double dist;

    std::tie(ueIdx, storedRnti, dist) = kv.second;

    if (storedRnti == rnti)
    {
      return true;
    }
  }
  return false;
}

void AddExtraUE(uint32_t nodeId, uint32_t ueIdx, uint16_t rnti, double distanceM)
{
  m_extaUeMap[nodeId] = std::make_tuple(ueIdx, rnti, distanceM);
}

bool HasRntiExtraUE(uint16_t rnti) const
{
  for (const auto& kv : m_extaUeMap)
  {
    uint32_t ueIdx;
    uint16_t storedRnti;
    double dist;

    std::tie(ueIdx, storedRnti, dist) = kv.second;

    if (storedRnti == rnti)
    {
      return true;
    }
  }
  return false;
}

void SetSchedulerCode(const std::string& code)
{
  m_schedulerCode = code;
}

std::string GetSchedulerCode() const
{
  return m_schedulerCode;
}

private:
  GlobalMetrics() 
  : m_round(0),
    m_seed(0),
    m_collectMetrics(false),
    m_outDir("fl-results/") 
    {}

  uint32_t m_round;                               
  uint32_t m_seed; 
  bool m_collectMetrics;  
  std::string m_outDir;  
  std::string m_schedulerCode;                            
  std::vector<uint32_t> m_successPerRound;         
  std::vector<uint32_t> m_rnitUELoad;
  std::map<uint32_t, uint64_t> m_ulRbgUeFLPerRound; 
  std::map<uint32_t, uint64_t> m_ulRbgUeNFLPerRound; 
  std::map<uint32_t, std::pair<uint64_t,uint64_t>> m_ulMCSUeFLPerRound;
  std::map<uint32_t, uint64_t> m_ulNumSymUeFLPerRound;
  std::map<uint32_t, uint64_t> m_ulTbSizeUeFLPerRound;
  std::map<uint32_t, uint64_t> m_schedCfgCntUeFLPerRound;
  std::map<uint32_t, std::pair<uint64_t,uint64_t>> m_ulNdiUeFLPerRound;
  std::map<uint32_t, std::vector<uint32_t>> m_successIdsPerRound; 
  std::map<uint32_t, std::vector<double>> m_successElapsedTimePerRound; 
  std::map<uint32_t, std::pair<double,double>> m_roundTimes; 
  std::map<uint32_t, std::pair<double,double>> m_roundWall;
  std::map<uint32_t, std::tuple<uint32_t, uint16_t, double>> m_ueMap; 
  std::map<uint32_t, std::tuple<uint32_t, uint16_t, double>> m_extaUeMap; 
};

} // namespace ns3::fl
