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
  FinalWeightAlg(const std::string& name, ISvcLocator* sl);
  StatusCode initialize() override;
  StatusCode execute(const EventContext& ctx) override;

private:
  CP::SysListHandle m_systematicsList{this};
  CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{this, "event", "EventInfo", "EventInfo container"};

  CP::SysWriteDecorHandle<float>
    m_weightHandle{this, "weightDecoration", "",
                   "per-systematic combined weight written to EventInfo"};
  Gaudi::Property<std::string> m_sysWeightName
    {this, "sysWeight", "",
     "Systematic-aware weight deco name (e.g. generatorWeight_%SYS%)"};
  Gaudi::Property<std::vector<std::string>> m_extraWeightNames
    {this, "extraWeights", {},
     "Non-systematic weight decoration names (ConstAccessor<float>)"};

  // created in initialize()
  std::unique_ptr<CP::SysReadDecorHandle<float>> m_sysWeightHandle;
  std::vector<SG::AuxElement::ConstAccessor<float>> m_extraWeightAccs;
};

} // namespace XBBCALIB
#endif
