#include "ns3/core-module.h"
#include "ns3/fl-global-metrics.h"
#include "nr-mac-scheduler-ofdma-fl.h"
#include "nr-mac-scheduler-ue-info-fl.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaFL");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaFL);

TypeId
NrMacSchedulerOfdmaFL::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NrMacSchedulerOfdmaFL")
                            .SetParent<NrMacSchedulerOfdma>()
                            .AddConstructor<NrMacSchedulerOfdmaFL>();
    return tid;
}

NrMacSchedulerOfdmaFL::NrMacSchedulerOfdmaFL()
    : NrMacSchedulerOfdma()
{
    std::cout << "--> NrMacSchedulerOfdmaFL" << std::endl;
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerOfdmaFL::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoFL>(
        params.m_rnti,
        params.m_beamId,
        std::bind(&NrMacSchedulerOfdmaFL::GetNumRbPerRbg, this));
}


void
NrMacSchedulerOfdmaFL::AssignedDlResources(const UePtrAndBufferReq& ue,
                                           [[maybe_unused]] const FTResources& assigned,
                                           [[maybe_unused]] const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    GetFirst GetUe;
    GetUe(ue)->UpdateDlMetric();
}

void
NrMacSchedulerOfdmaFL::AssignedUlResources(const UePtrAndBufferReq& ue,
                                           [[maybe_unused]] const FTResources& assigned,
                                           [[maybe_unused]] const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    GetFirst GetUe;
    GetUe(ue)->UpdateUlMetric();
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaFL::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoFL::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaFL::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoFL::CompareUeWeightsUl;
}

} // namespace ns3
