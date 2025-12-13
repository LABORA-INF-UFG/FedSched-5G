#pragma once
#include "nr-mac-scheduler-ns3.h"
#include "nr-mac-scheduler-ue-info-rr.h"

namespace ns3
{

class NrMacSchedulerUeInfoFL : public NrMacSchedulerUeInfo
{
  public:
    NrMacSchedulerUeInfoFL(uint16_t rnti, BeamId beamId, const GetRbPerRbgFn& fn)
        : NrMacSchedulerUeInfo(rnti, beamId, fn)
    {        
    }
 
    static bool CompareUeWeightsDl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        if (lue.first->GetDlMcs() == rue.first->GetDlMcs())
        {
            return NrMacSchedulerUeInfoRR::CompareUeWeightsDl(lue, rue);
        }

        return (lue.first->GetDlMcs() > rue.first->GetDlMcs());
    }

    static bool CompareUeWeightsUl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        const bool lFl = lue.first->m_ueExtInfo->GetIsFL();
        const bool rFl = rue.first->m_ueExtInfo->GetIsFL();
       
        if (lFl != rFl) 
            return lFl;
                
        const uint8_t lM = lue.first->m_ulMcs;
        const uint8_t rM = rue.first->m_ulMcs;

        if (lM == rM)
            return NrMacSchedulerUeInfoRR::CompareUeWeightsUl(lue, rue);

        return lM > rM;
    }
    
};

} // namespace ns3
