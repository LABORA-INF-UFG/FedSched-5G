#pragma once
#include "nr-mac-scheduler-ue-ext-info.h"
#include "nr-mac-scheduler-ue-load-ext-info.h" 
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/type-id.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerUeExtInfo");
NS_OBJECT_ENSURE_REGISTERED (NrMacSchedulerUeExtInfo);

TypeId
NrMacSchedulerUeExtInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrMacSchedulerUeExtInfo")
    .SetParent<Object> ()
    .AddConstructor<NrMacSchedulerUeExtInfo> ()
    .AddAttribute ("IsFL",
                   "Indica se o UE participa de FL.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&NrMacSchedulerUeExtInfo::m_isFL),
                   MakeBooleanChecker ());
  return tid;
}

NrMacSchedulerUeExtInfo::NrMacSchedulerUeExtInfo () = default;
NrMacSchedulerUeExtInfo::~NrMacSchedulerUeExtInfo () = default;

void
NrMacSchedulerUeExtInfo::SetIsFL (uint16_t rnti)
{
  const auto& s = NrMacSchedulerUeLoadExtInfo::FLUeSet;
  m_isFL = (s.find(rnti) != s.end());  
}

bool
NrMacSchedulerUeExtInfo::GetIsFL () const
{
  return m_isFL;
}

} // namespace ns3
