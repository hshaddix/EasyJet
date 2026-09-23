/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "FinalWeightAlg.h"

namespace XBBCALIB {

StatusCode FinalWeightAlg::initialize() {
  if (m_weightHandle.empty()) {
    ATH_MSG_ERROR("weightDecoration must be configured");
    return StatusCode::FAILURE;
  }
  ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
  ATH_CHECK(m_weightHandle.initialize(m_systematicsList, m_eventHandle));

  for (const auto &n : m_sysWeightNames.value()) {
    auto h = std::make_unique<CP::SysReadDecorHandle<float>>(n, this);
    ATH_CHECK(h->initialize(m_systematicsList, m_eventHandle));
    m_sysWeightsHandle.push_back(std::move(h));
  }
  for (const auto& n : m_extraWeightNames.value())
    m_extraWeightAccs.emplace_back(n);

  ATH_CHECK(m_systematicsList.initialize());
  return StatusCode::SUCCESS;
}

StatusCode FinalWeightAlg::execute(const EventContext& ctx) {
  for (const auto& sys : m_systematicsList.systematicsVector()) {
    const xAOD::EventInfo* event = nullptr;
    ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

    float w = 1.f;
    for (auto & weight_handle : m_sysWeightsHandle) {
      w *= weight_handle->get(*event, sys);
    }
    for (const auto& acc : m_extraWeightAccs)
      w *= acc(*event);
    m_weightHandle.set(*event, w, sys);
  }
  return StatusCode::SUCCESS;
}

} // namespace XBBCALIB
