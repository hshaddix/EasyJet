/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZtautauCalibSelectorAlg.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthenaKernel/Units.h>
#include <TVector2.h>
#include <cmath>
#include <limits>

namespace XBBCALIB
{
  StatusCode ZtautauCalibSelectorAlg::initialize()
  {
    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    ATH_CHECK (m_lRjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_metHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK (m_outHandle.initialize(m_systematicsList));
    ATH_CHECK (m_zcandMetMassHandle.initialize(m_systematicsList, m_outHandle));
    ATH_CHECK (m_zcandTruthJetHandle.initialize(m_systematicsList, m_outHandle));
    if (m_isMC) {
      ATH_CHECK (m_truthJetHandle.initialize(m_systematicsList));
    }

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize()); 

    return StatusCode::SUCCESS;
  }

  // The Z candidate is the large-R jet with the smallest |Delta Phi| 
  // with respect to the MET direction. The other jet (assumed to be the only other entry, 
  // since baseline requires exactly 2 large-R jets) is taken as the recoiling jet.
  void ZtautauCalibSelectorAlg::findZcandIndex(
    const xAOD::JetContainer& jets,
    float met_phi,
    size_t& zcandIdx,
    size_t& recoilIdx
  ) {
    zcandIdx = 0;
    float bestDeltaPhi = std::numeric_limits<float>::max();
    for (size_t i = 0; i < jets.size(); ++i) {
      const float dphi = std::abs(TVector2::Phi_mpi_pi(jets.at(i)->phi() - met_phi));
      if (dphi < bestDeltaPhi) {
        bestDeltaPhi = dphi;
        zcandIdx = i;
      }
    }
    recoilIdx = (zcandIdx == 0) ? 1 : 0;
  }

  // Compute collinear MET correction:
  //   alpha = (Zcand.pt * met.pt * cos(DeltaPhi(Zcand,met))) / Zcand.pt^2
  //         = (met.pt / Zcand.pt) * cos(DeltaPhi(Zcand,met))
  //   Zcand_met_mass = Zcand.mass * (1 + alpha)   if alpha >= 0
  //                  = defaultValue                otherwise (collinear approx only
  //                                                  valid for |DeltaPhi(Zcand,met)| < pi/2)
  float ZtautauCalibSelectorAlg::computeZcandMetMass(
    const xAOD::Jet& zcandJet,
    const xAOD::MissingET& met,
    float defaultValue
  ) {
    if (zcandJet.pt() <= 0) return defaultValue;

    const float dphi = TVector2::Phi_mpi_pi(zcandJet.phi() - met.phi());
    const float alpha = (met.met() / zcandJet.pt()) * std::cos(dphi);

    if (alpha < 0 or alpha > 1) return defaultValue;
    return zcandJet.m() * (1 + alpha);
  }

  // Tau1_wta/Tau2_wta are static substructure moments stored on the jet
  // (not systematic-dependent), so they're read as plain aux data.
  float ZtautauCalibSelectorAlg::computeT21(const xAOD::Jet& jet)
  {
    static const SG::ConstAccessor<float> tau1WtaAcc("Tau1_wta");
    static const SG::ConstAccessor<float> tau2WtaAcc("Tau2_wta");
 
    const float tau1_wta = tau1WtaAcc(jet);
    const float tau2_wta = tau2WtaAcc(jet);
    return tau2_wta / tau1_wta;
  }

  // Find the truth jet closest in DeltaR to recoJet; -1 if truthJets is empty
  // or the closest one is farther than maxDeltaR.
  int ZtautauCalibSelectorAlg::findTruthMatch(
    const xAOD::Jet& recoJet,
    const xAOD::JetContainer& truthJets,
    float maxDeltaR
  ) {
    int bestIdx = -1;
    float bestDeltaR = std::numeric_limits<float>::max();
    for (size_t i = 0; i < truthJets.size(); ++i) {
      const float dR = recoJet.p4().DeltaR(truthJets.at(i)->p4());
      if (dR < bestDeltaR) {
        bestDeltaR = dR;
        bestIdx = static_cast<int>(i);
      }
    }
    if (bestIdx < 0 || bestDeltaR > maxDeltaR) return -1;
    return bestIdx;
  }


  StatusCode ZtautauCalibSelectorAlg::execute(const EventContext& ctx)
  {

    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Initialize Variables
      bool pass_baseline = false;

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));
      const xAOD::JetContainer *lRjets = nullptr;
      ANA_CHECK (m_lRjetHandle.retrieve (lRjets, sys, ctx));
      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK(m_electronHandle.retrieve (electrons, sys, ctx));
      const xAOD::MissingETContainer *metCont = nullptr;
      ANA_CHECK(m_metHandle.retrieve (metCont, sys, ctx));

      // Create output view container (always written, even if event fails)
      auto ordered = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);

      //************
      // Large-R Jet
      //************
      int n_boosted_jets = lRjets->size();

      //************
      // electrons
      //************
      int n_electrons = electrons->size();

      //************
      // MET
      //************
      // post retrieval checks - here is only a met check
      // in principle. we can do any checks
      float met_phi = 0;
      const xAOD::MissingET *met = (*metCont)["Final"];
      if (!met) {
        ATH_MSG_ERROR("Could not retrieve MET");
        return StatusCode::FAILURE;
      }
      met_phi = met->phi();

      //************
      // Apply Selection
      //************
      float deltaPhi_largeR = 0;
      if (n_boosted_jets >= 2) {
        const xAOD::Jet *j0 = lRjets->at(0);
        const xAOD::Jet *j1 = lRjets->at(1);
        deltaPhi_largeR = j0->p4().DeltaPhi(j1->p4());
      }

      // Need to have a at least 2 large-R jets with DPhi > 1.5, no electrons
      // and MET > 30 GeV.
      pass_baseline = (
        n_boosted_jets == 2 and
        n_electrons == 0 and
        std::abs(deltaPhi_largeR) >= 1.5 and
        met->met() > 30
      );

      if (!m_bypass && !pass_baseline){
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys, ctx));
        continue;
      }

      // Select the Z candidate as the large-R jet with the smallest |Delta Phi|
      // w.r.t. MET; the other jet is the recoiling jet.
      size_t zcandIdx = 0;
      size_t recoilIdx = 1;
      findZcandIndex(*lRjets, met_phi, zcandIdx, recoilIdx);

      // The Z-candidate jet must be sufficiently 2-pronged
      // (t21 = Tau2_wta/Tau1_wta <= 0.2).
      float zcand_t21 = computeT21(*lRjets->at(zcandIdx));
      if (zcand_t21 > 0.2){
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys, ctx));
        continue;
      }

      // Zcand mass corrected with MET in the collinear approximation, decorated
      // directly onto the jet so downstream algorithms can read it back without
      // needing to retrieve MET themselves.
      const xAOD::Jet *zcandJet = lRjets->at(zcandIdx);
      const float zcand_met_mass = computeZcandMetMass(*zcandJet, *met, -99.);
      if (zcand_met_mass == -99.){
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys, ctx));
        continue;
      }
      m_zcandMetMassHandle.set(*zcandJet, zcand_met_mass, sys);

      // Truth jet (nearest in DeltaR) matched to the Z candidate, decorated as
      // a single JetFourMom_t block directly onto the jet (MC only; default
      // (-99,-99,-99,-99) for data or no match within truthMatchMaxDeltaR).
      xAOD::JetFourMom_t truth_p4;
      if (m_isMC) {
        const xAOD::JetContainer *truthJets = nullptr;
        ANA_CHECK (m_truthJetHandle.retrieve (truthJets, sys, ctx));
        const int matchIdx = findTruthMatch(*zcandJet, *truthJets, 1.0);
        if (matchIdx >= 0) {
          const xAOD::Jet *truthJet = truthJets->at(matchIdx);
          truth_p4 = xAOD::JetFourMom_t(truthJet->pt(), truthJet->eta(), truthJet->phi(), truthJet->m());
        }
      }
      m_zcandTruthJetHandle.set(*zcandJet, truth_p4, sys);

      // Write out the two jets in order: [0] = Z candidate, [1] = recoiling jet
      ordered->push_back(lRjets->at(zcandIdx));
      ordered->push_back(lRjets->at(recoilIdx));
      ANA_CHECK (m_outHandle.record (std::move(ordered), sys, ctx));

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZtautauCalibSelectorAlg::finalize()
  {
    ANA_CHECK (m_filterParams.finalize());
    return StatusCode::SUCCESS;
  }

}
