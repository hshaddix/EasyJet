/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef XBBCALIB_FINALSELECTIONALGC_H
#define XBBCALIB_FINALSELECTIONALGC_H

#include <AthenaBaseComps/AthAlgorithm.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <xAODEventInfo/EventInfo.h>
#include <AthContainers/AuxElement.h>
#include <memory>
#include <optional>
#include <string>

namespace XBBCALIB {

struct cutvals_t;

class FinalSelectionAlg final : public AthAlgorithm {
public:
  FinalSelectionAlg(const std::string& name, ISvcLocator* sl);
  ~FinalSelectionAlg();
  StatusCode initialize() override;
  StatusCode execute() override;

private:
  CP::SysListHandle m_systematicsList{this};
  CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{this, "event", "EventInfo", "EventInfo container"};

  // Written decoration — name must contain %SYS%
  CP::SysWriteDecorHandle<char>
    m_passHandle{this, "selectionDecoration",
                 "",
                 "per-systematic pass/fail char on EventInfo"};

  // Input decorations — created dynamically in initialize() if cut is configured
  std::shared_ptr<CP::SysReadDecorHandle<int>>   m_lrjetsNHandle;

  // Optional Xbb-WP cut (empty = no cut).
  Gaudi::Property<std::string> m_tagDecoName
    {this, "tagDecoration", "",
     "EventInfo int decoration for WP tag cut; non-zero => pass"};
  std::unique_ptr<CP::SysReadDecorHandle<int>> m_tagDecoHandle;

  // JSON string carrying the selection thresholds
  Gaudi::Property<std::string> m_selectionConfigJson
    {this, "selectionConfig", "{}", "JSON object: selection thresholds"};

  // Blinding: sets pass=false when variable value is in (min, max)
  Gaudi::Property<std::string> m_blindingVariable
    {this, "blindingVariable", "",
     "EventInfo float deco for blinding check (_%SYS% appended internally)"};
  Gaudi::Property<float> m_blindingMin{this, "blindingMin", 0.f};
  Gaudi::Property<float> m_blindingMax{this, "blindingMax", 0.f};

  // Parsed thresholds (populated in initialize())
  std::unique_ptr<cutvals_t> m_cuts;

  // Handles created in initialize()
  std::unique_ptr<CP::SysReadDecorHandle<float>> m_blindingHandle;
};

} // namespace XBBCALIB
#endif
