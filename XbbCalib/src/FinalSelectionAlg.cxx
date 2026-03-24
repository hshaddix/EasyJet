/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "FinalSelectionAlg.h"
#include <boost/json.hpp>

namespace XBBCALIB {

FinalSelectionAlg::FinalSelectionAlg(const std::string& name, ISvcLocator* sl)
  : AthHistogramAlgorithm(name, sl) {}

StatusCode FinalSelectionAlg::initialize() {
  if (m_passHandle.empty()) {
    ATH_MSG_ERROR("selectionDecoration must be configured");
    return StatusCode::FAILURE;
  }
  // Parse JSON config
  const auto obj = boost::json::parse(m_selectionConfigJson.value()).as_object();
  if (const auto* v = obj.if_contains("n_lrjets_min"))
    m_n_lrjets_min = static_cast<int>(v->as_int64());
  if (const auto* v = obj.if_contains("Zcand_m_min"))
    m_Zcand_m_min = static_cast<float>(v->as_double());
  if (const auto* v = obj.if_contains("Zcand_m_max"))
    m_Zcand_m_max = static_cast<float>(v->as_double());
  if (const auto* v = obj.if_contains("sample_label"))
    m_sample_label = std::string(v->as_string());

  // event handle must be initialised before decoration handles that reference it
  ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
  ATH_CHECK(m_passHandle.initialize(m_systematicsList, m_eventHandle));

  if (m_n_lrjets_min.has_value()) {
    m_lrjetsNHandle = std::make_shared<CP::SysReadDecorHandle<int>>(
      "lrjets_n_%SYS%", this);
    ATH_CHECK(m_lrjetsNHandle->initialize(m_systematicsList, m_eventHandle));
  }
  if (m_Zcand_m_min.has_value() || m_Zcand_m_max.has_value()) {
    m_zcandMHandle = std::make_shared<CP::SysReadDecorHandle<float>>(
      "Zcand_m_%SYS%", this);
    ATH_CHECK(m_zcandMHandle->initialize(m_systematicsList, m_eventHandle));
  }

  if (!m_tagDecoName.value().empty()) {
    m_tagDecoHandle = std::make_unique<CP::SysReadDecorHandle<int>>(
      m_tagDecoName.value(), this);
    ATH_CHECK(m_tagDecoHandle->initialize(m_systematicsList, m_eventHandle));
  }

  ATH_CHECK(m_systematicsList.initialize());
  return StatusCode::SUCCESS;
}

StatusCode FinalSelectionAlg::execute() {
  for (const auto& sys : m_systematicsList.systematicsVector()) {
    const xAOD::EventInfo* event = nullptr;
    ANA_CHECK(m_eventHandle.retrieve(event, sys));

    bool pass = true;

    if (m_n_lrjets_min.has_value())
      pass = pass && (m_lrjetsNHandle->get(*event, sys) >= *m_n_lrjets_min);

    if (m_Zcand_m_min.has_value())
      pass = pass && (m_zcandMHandle->get(*event, sys) >= *m_Zcand_m_min);

    if (m_Zcand_m_max.has_value())
      pass = pass && (m_zcandMHandle->get(*event, sys) <= *m_Zcand_m_max);

    if (m_tagDecoHandle)
      pass = pass && (m_tagDecoHandle->get(*event, sys) != 0);

    m_passHandle.set(*event, static_cast<char>(pass), sys);
  }
  return StatusCode::SUCCESS;
}

} // namespace XBBCALIB
