#pragma once
#include "ns3/object.h"
#include "ns3/type-id.h"

namespace ns3
{

class NrMacSchedulerUeExtInfo : public Object
{
public:
  static TypeId GetTypeId (void);

  NrMacSchedulerUeExtInfo ();
  virtual ~NrMacSchedulerUeExtInfo ();

  void SetIsFL(uint16_t rnti);
  bool GetIsFL () const;

  uint16_t m_qtde_round{0};


private:
  bool     m_isFL {false};
};

} // namespace ns3
