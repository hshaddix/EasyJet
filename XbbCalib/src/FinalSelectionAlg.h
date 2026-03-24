/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef XBBCALIB_FINALSELECTIONALGC_H
#define XBBCALIB_FINALSELECTIONALGC_H

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <xAODEventInfo/EventInfo.h>
#include <memory>
#include <optional>
#include <string>

namespace XBBCALIB {

class FinalSelectionAlg final : public AthHistogramAlgorithm {
public:
  FinalSelectionAlg(const std::string& name, ISvcLocator* sl);
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
  std::shared_ptr<CP::SysReadDecorHandle<float>>  m_zcandMHandle;

  // Optional Xbb-WP cut (empty = no cut).
  Gaudi::Property<std::string> m_tagDecoName
    {this, "tagDecoration", "",
     "EventInfo int decoration for WP tag cut; non-zero => pass"};
  std::unique_ptr<CP::SysReadDecorHandle<int>> m_tagDecoHandle;

  // JSON string carrying the selection thresholds
  Gaudi::Property<std::string> m_selectionConfigJson
    {this, "selectionConfig", "{}", "JSON object: selection thresholds"};

  // Parsed thresholds (populated in initialize())
  std::optional<int>   m_n_lrjets_min;
  std::optional<float> m_Zcand_m_min;
  std::optional<float> m_Zcand_m_max;
  std::string          m_sample_label;   // string example — metadata, no cut
};

} // namespace XBBCALIB
#endif
