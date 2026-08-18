/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetDecoratorAlg.h"
#include <AsgDataHandles/ReadDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandle.h>


namespace Easyjet
{
  StatusCode JetDecoratorAlg::initialize()
  {
    ATH_CHECK(m_jetsInKey.initialize());

    // truth matching
    ATH_CHECK(m_truthJetsInKey.initialize(m_isMC));
    m_truthLabelDecorKey = m_truthJetsInKey.key() + ".HadronConeExclTruthLabelID";
    ATH_CHECK(m_truthLabelDecorKey.initialize(m_isMC));

    m_bJetTruthPtDecorKey = m_jetsInKey.key() + ".bJetTruthPt";
    m_bJetTruthDRDecorKey = m_jetsInKey.key() + ".bJetTruthDR";
    ATH_CHECK(m_bJetTruthPtDecorKey.initialize(m_isMC));
    ATH_CHECK(m_bJetTruthDRDecorKey.initialize(m_isMC));

    return StatusCode::SUCCESS;
  }

  StatusCode JetDecoratorAlg ::execute(const EventContext& ctx) const
  {
    SG::ReadHandle<xAOD::JetContainer> jets(m_jetsInKey,ctx);
    ATH_CHECK(jets.isValid());

    if (m_isMC) {
      SG::ReadHandle<xAOD::JetContainer> truthJets(m_truthJetsInKey, ctx);
      ATH_CHECK(truthJets.isValid());

      SG::ReadDecorHandle<xAOD::JetContainer, int> truthLabel(m_truthLabelDecorKey, ctx);
      SG::WriteDecorHandle<xAOD::JetContainer, float> bJetTruthPt(m_bJetTruthPtDecorKey, ctx);
      SG::WriteDecorHandle<xAOD::JetContainer, float> bJetTruthDR(m_bJetTruthDRDecorKey, ctx);

      for(const xAOD::Jet* jet: *jets) {
        float minDR = m_minDR;
        const xAOD::Jet *bestTruth = nullptr;

        for (const xAOD::Jet *truthJet : *truthJets)
        {
          if (truthJet->pt() < m_minTruthPt)
            continue;
          if (truthLabel(*truthJet) != 5)
            continue;

          float dR = jet->p4().DeltaR(truthJet->p4());
          if (dR < minDR)
          {
            bestTruth = truthJet;
            minDR = dR;
          }
        }

        bJetTruthPt(*jet) = bestTruth ? bestTruth->pt() : -99.;
        bJetTruthDR(*jet) = bestTruth ? minDR : -99.;
      }
    }

    return StatusCode::SUCCESS;
  }

}
