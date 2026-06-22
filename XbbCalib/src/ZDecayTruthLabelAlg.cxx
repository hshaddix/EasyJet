/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#include "ZDecayTruthLabelAlg.h"
#include <StoreGate/ReadHandle.h>
#include <StoreGate/WriteDecorHandle.h>

namespace XBBCALIB {

ZDecayTruthLabelAlg::ZDecayTruthLabelAlg(const std::string& name,
                                          ISvcLocator* sl)
  : AthAlgorithm(name, sl) {}

StatusCode ZDecayTruthLabelAlg::initialize() {
  ATH_CHECK(m_eventKey.initialize());
  ATH_CHECK(m_isCC_key.initialize());
  ATH_CHECK(m_truthKey.initialize());
  return StatusCode::SUCCESS;
}

StatusCode ZDecayTruthLabelAlg::execute(const EventContext& ctx) {

  // Classify Z decay from truth: any charm quark (|pdgId|==4) means Z->cc.
  SG::ReadHandle<xAOD::TruthParticleContainer> truth(m_truthKey, ctx);
  int z_is_cc = 0;
  for (const xAOD::TruthParticle* p : *truth) {
    if (std::abs(p->pdgId()) == 4) { z_is_cc = 1; break; }
  }

  SG::ReadHandle<xAOD::EventInfo> ei(m_eventKey, ctx);
  SG::WriteDecorHandle<xAOD::EventInfo, int> decor(m_isCC_key, ctx);
  decor(*ei) = z_is_cc;
  return StatusCode::SUCCESS;
}

} // namespace XBBCALIB
