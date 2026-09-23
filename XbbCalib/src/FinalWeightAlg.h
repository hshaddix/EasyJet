/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/
#ifndef XBBCALIB_FINALWEIGHTALG_H
#define XBBCALIB_FINALWEIGHTALG_H

#include <AthenaBaseComps/AthAlgorithm.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <AthContainers/AuxElement.h>
#include <xAODEventInfo/EventInfo.h>
#include <memory>
#include <string>
#include <vector>

namespace XBBCALIB {

class FinalWeightAlg final : public AthAlgorithm {
public:
  using AthAlgorithm::AthAlgorithm;

  StatusCode initialize() override;
  StatusCode execute(const EventContext& ctx) override;

private:
  CP::SysListHandle m_systematicsList{this};
  CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{this, "event", "EventInfo", "EventInfo container"};

  CP::SysWriteDecorHandle<float>
    m_weightHandle{this, "weightDecoration", "",
                   "per-systematic combined weight written to EventInfo"};
  Gaudi::Property<std::vector<std::string>> m_sysWeightNames
    { this,"sysWeights",  {},
     "Systematic-aware weight deco name patterns (must contain %SYS%)"};
  Gaudi::Property<std::vector<std::string>> m_extraWeightNames
    {this, "extraWeights", {},
     "Non-systematic weight decoration names (ConstAccessor<float>)"};

  // created in initialize()
  std::vector<std::unique_ptr<CP::SysReadDecorHandle<float>>> m_sysWeightsHandle;
  std::vector<SG::AuxElement::ConstAccessor<float>> m_extraWeightAccs;
};

} // namespace XBBCALIB
#endif
