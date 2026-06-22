/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "BaselineVarsFullHadAlg.h"
#include "AthContainers/AuxElement.h"
#include "TLorentzVector.h"
#include <cmath>
#include <array>
#include <limits>
#include "AthContainers/ConstDataVector.h"
#include <AthenaKernel/Units.h>

namespace {

constexpr float Wmass = 80.379f*Athena::Units::GeV; // W boson mass
constexpr float Zmass = 91.1876f*Athena::Units::GeV; // Z boson mass

struct BosonPairingResult {
  bool valid{false};
  float score{std::numeric_limits<float>::max()};
  TLorentzVector candidate1;
  TLorentzVector candidate2;
};

BosonPairingResult findBestBosonPairing(const std::vector<const xAOD::Jet*>& jets,
                                        float targetMass)
{

  // For the resolved vector boson case, where both vector bosons are both reconstructed with AK4 jets.
  BosonPairingResult best;
  if (jets.size() < 4) {
    return best;
  }

  for (std::size_t i = 0; i + 3 < jets.size(); ++i) {
    for (std::size_t j = i + 1; j + 2 < jets.size(); ++j) {
      for (std::size_t k = j + 1; k + 1 < jets.size(); ++k) {
        for (std::size_t l = k + 1; l < jets.size(); ++l) {
          const std::array<std::array<std::size_t, 4>, 3> pairings{{
            {{i, j, k, l}},
            {{i, k, j, l}},
            {{i, l, j, k}}
          }};

          for (const auto& pairing : pairings) {
            const TLorentzVector candidate1 = jets[pairing[0]]->p4() + jets[pairing[1]]->p4();
            const TLorentzVector candidate2 = jets[pairing[2]]->p4() + jets[pairing[3]]->p4();
            const float score = std::abs(candidate1.M() - targetMass)
                              + std::abs(candidate2.M() - targetMass);

            if (score < best.score) {
              best.valid = true;
              best.score = score;
              if (candidate1.Pt() >= candidate2.Pt()) {
                best.candidate1 = candidate1;
                best.candidate2 = candidate2;
              } else {
                best.candidate1 = candidate2;
                best.candidate2 = candidate1;
              }
            }
          }
        }
      }
    }
  }

  return best;
}

} // namespace

namespace VBSHIGGS {

  BaselineVarsFullHadAlg::BaselineVarsFullHadAlg(
    const std::string& name, ISvcLocator* pSvcLocator)
    : AthHistogramAlgorithm(name, pSvcLocator) {}

  StatusCode BaselineVarsFullHadAlg::initialize() {

    ATH_MSG_INFO("*********************************");
    ATH_MSG_INFO("       FullHadBaselineVarsAlg    ");
    ATH_MSG_INFO("*********************************");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_vbsLRJetHandle.initialize(m_systematicsList));
    
    if (m_doResolved) {
      ATH_CHECK(m_signaljetHandle.initialize(m_systematicsList));
      ATH_CHECK(m_HCandHandle.initialize(m_systematicsList));
      if (!m_isBtag.empty()) {
        ATH_CHECK(m_isBtag.initialize(m_systematicsList, m_signaljetHandle));
      }
    }

          
    if( !m_UseVBFRNN ){
      ATH_CHECK (m_vbsJetHandle.initialize(m_systematicsList));
    }
    else {
      ATH_CHECK (m_RNNjetBoostedHandle.initialize(m_systematicsList));
      if (m_doResolved) 
        ATH_CHECK (m_RNNjetResolvedHandle.initialize(m_systematicsList));
    }




    for (const auto& var : m_floatVariables) {
      CP::SysWriteDecorHandle<float> h(var + "_%SYS%", this);
      m_Fbranches.emplace(var, h);
      ATH_CHECK(m_Fbranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    for (const auto& var : m_intVariables) {
      CP::SysWriteDecorHandle<int> h(var + "_%SYS%", this);
      m_Ibranches.emplace(var, h);
      ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    m_WTag_score = CP::SysReadDecorHandle<float> (m_WTag_Type.value()+m_WTag_WP.value()+"Tagger_Score", this);
    m_Pass_WTag = CP::SysReadDecorHandle<char> (m_WTag_Type.value()+m_WTag_WP.value()+"Tagger_Tagged", this);
    ATH_CHECK(m_WTag_score.initialize(m_systematicsList, m_vbsLRJetHandle));
    ATH_CHECK(m_Pass_WTag.initialize(m_systematicsList, m_vbsLRJetHandle));

    ATH_CHECK(m_systematicsList.initialize());
    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsFullHadAlg::execute(const EventContext& /*ctx*/) {

    for (const auto& sys : m_systematicsList.systematicsVector()) {

      const xAOD::EventInfo* event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys));

      const xAOD::JetContainer* largeJets = nullptr;
      ANA_CHECK(m_vbsLRJetHandle.retrieve(largeJets, sys));

      const xAOD::JetContainer *signalJets = nullptr;    
      const xAOD::JetContainer *HJets = nullptr;
      if (m_doResolved){
        ANA_CHECK (m_signaljetHandle.retrieve (signalJets, sys));
        ANA_CHECK (m_HCandHandle.retrieve (HJets, sys));
      }

      
      const xAOD::JetContainer *vbsjets = nullptr;
      const xAOD::JetContainer *RNNJets_boosted= nullptr;
      const xAOD::JetContainer *RNNJets_resolved = nullptr;
      if(!m_UseVBFRNN) {
        ANA_CHECK (m_vbsJetHandle.retrieve (vbsjets, sys));
      }
      else {
        ANA_CHECK (m_RNNjetBoostedHandle.retrieve (RNNJets_boosted, sys));
        if (m_doResolved)
    ANA_CHECK (m_RNNjetResolvedHandle.retrieve (RNNJets_resolved, sys));
      }



      for (const auto& v : m_floatVariables)
        m_Fbranches.at(v).set(*event, -99.f, sys);
      for (const auto& v : m_intVariables)
        m_Ibranches.at(v).set(*event, -99, sys);

      int nCentralJets = 0;
      int nForwardJets = 0;
      int nLargeJets   = largeJets->size();
      bool WPgiven = !m_isBtag.empty();
      if (m_doResolved) {

    int n_jets = signalJets->size();
    if (!m_UseVBFRNN && vbsjets)
      n_jets += vbsjets->size();

    auto bjets = std::make_unique<
        ConstDataVector<xAOD::JetContainer>
    >(SG::VIEW_ELEMENTS);

    int nBJets = 0;

    for (const xAOD::Jet* jet : *signalJets) {

      if (std::abs(jet->eta()) < 2.5)
        nCentralJets++;
      else
        nForwardJets++;

      if (WPgiven &&
          std::abs(jet->eta()) < 2.5 &&
          m_isBtag.get(*jet, sys)) {

        bjets->push_back(jet);
        nBJets++;
      }
    }

    if (!m_UseVBFRNN && vbsjets) {
      for (const xAOD::Jet* vbsjet : *vbsjets) {
        if (std::abs(vbsjet->eta()) < 2.5)
          nCentralJets++;
        else
          nForwardJets++;
      }
    }

    m_Ibranches.at("nJets").set(*event, n_jets, sys);
    m_Ibranches.at("nBJets").set(*event, nBJets, sys);
    m_Ibranches.at("nCentralJets").set(*event, nCentralJets, sys);
    m_Ibranches.at("nForwardJets").set(*event, nForwardJets, sys);
    m_Ibranches.at("nLargeJets").set(*event, nLargeJets, sys);
  }



  ////////////////////////////////////// W Tagger scores //////////////////////////////////////////////
    for(unsigned int i=0; i<std::min(size_t(3),largeJets->size()); i++){
      const xAOD::Jet* largeJet = largeJets->at(i);
      std::string prefix = "LargeJet"+std::to_string(i+1);
      // Get score of W-tagger
      float wtagger_score = m_WTag_score.get(*largeJet, sys);
      m_Fbranches.at(prefix+"_m").set(*event, largeJet->m(), sys);
      m_Fbranches.at(prefix+"_pt").set(*event, largeJet->pt(), sys);
      m_Fbranches.at(prefix+"_eta").set(*event, largeJet->eta(), sys);
      m_Fbranches.at(prefix+"_phi").set(*event, largeJet->phi(), sys);
      m_Fbranches.at(prefix + "_" + m_WTag_Type + m_WTag_WP + "Tagger_Score").set(*event, wtagger_score, sys);
      int pass_wtag = static_cast<int>(m_Pass_WTag.get(*largeJet, sys));
      m_Ibranches.at(prefix+"_Pass_"+m_WTag_Type+m_WTag_WP).set(*event, pass_wtag, sys);
    }

    
    //b-jet sector
    ///////////////////////////////// Resolved Higgs->bb candidates /////////////////////////////////////////////

    if (m_doResolved) {
      TLorentzVector HDijet;
      TLorentzVector HJet1;
      TLorentzVector HJet2;
      for (std::size_t i=0; i<std::min(HJets->size(),(std::size_t)2); i++){
        std::string prefix = "Jet_Higgs_candidate"+std::to_string(i+1);
        m_Fbranches.at(prefix+"_m").set(*event, HJets->at(i)->m(), sys);
        m_Fbranches.at(prefix+"_pt").set(*event, HJets->at(i)->pt(), sys);
        m_Fbranches.at(prefix+"_eta").set(*event, HJets->at(i)->eta(), sys);
        m_Fbranches.at(prefix+"_phi").set(*event, HJets->at(i)->phi(), sys);
        m_Fbranches.at(prefix+"_E").set(*event, HJets->at(i)->e(), sys);

        }

      if (HJets->size() >=2){
        HDijet = HJets->at(0)->p4()+ HJets->at(1)->p4();
        HJet1 = HJets->at(0)->p4();
        HJet2 = HJets->at(1)->p4();
        // std::cout << "Higgs dijet mass: " << HDijet.M() << std::endl;
        m_Fbranches.at("Hdijet_m").set(*event, HDijet.M(), sys);
        m_Fbranches.at("Hdijet_pt").set(*event, HDijet.Pt(), sys);
        m_Fbranches.at("Hdijet_eta").set(*event, HDijet.Eta(), sys);
        m_Fbranches.at("Hdijet_phi").set(*event, HDijet.Phi(), sys);
        m_Fbranches.at("dRHjj").set(*event, HJet1.DeltaR(HJet2), sys);
        m_Fbranches.at("dPhiHjj").set(*event, HJet1.DeltaPhi(HJet2), sys);
        m_Fbranches.at("dEtaHjj").set(*event, HJet1.Eta() - HJet2.Eta(), sys);
      }
    }

    // Two W/Z vector bosons. Two cases for the Higgs: boosted H, resolved H
    // For W/Z vector bosons. It can also be either boosted or resolved.
    // For kl, we expect less than 3 large R jets. For k2v, expect 3 or more large R jets. 
  //////////////////////////////////////// Resolved Higgs->bb with Resolved V jets /////////////////////////////////////
    // auto resolvedVCandidates_resolvedH = std::make_unique<ConstDataVector<xAOD::JetContainer>> (SG::VIEW_ELEMENTS);

    // resovledVCandidates_boostedH also works for the case where there are 2 large R jets. Higgs boosted V1 boosted and V2 resolved. 
    // The remaining non b-tagged jets that are not matched to the Higgs will be used to reconstruct the V boson candidates.
    if (m_doResolved) {
      std::vector<const xAOD::Jet*> resolvedVCandidates_boostedH;
          for(const xAOD::Jet* jet : *signalJets) {
            // only central jets          
            if (std::abs(jet->eta())>=2.5) continue;
            // non b-tagged jets 
            // Don't include b-tagged jets for W boson jet candidates. However, for Z, bjets make up a nontrivial fraction of the candidates. B(Z->bb) ~ 15%. 
            // Assume Z->bb is negligible
            if ( !m_isBtag.empty() && m_isBtag.get(*jet, sys) ) continue;
            resolvedVCandidates_boostedH.push_back(jet);
          }
          const auto resolvedWCandidates = findBestBosonPairing(resolvedVCandidates_boostedH,
                                                                Wmass);
          const auto resolvedZCandidates = findBestBosonPairing(resolvedVCandidates_boostedH,
                                                                Zmass);
          if (resolvedWCandidates.valid){
            m_Fbranches.at("resolvedWCandidate1_m").set(*event, resolvedWCandidates.candidate1.M(), sys);
            m_Fbranches.at("resolvedWCandidate1_pt").set(*event, resolvedWCandidates.candidate1.Pt(), sys);
            m_Fbranches.at("resolvedWCandidate1_eta").set(*event, resolvedWCandidates.candidate1.Eta(), sys);
            m_Fbranches.at("resolvedWCandidate1_phi").set(*event, resolvedWCandidates.candidate1.Phi(), sys);
            m_Fbranches.at("resolvedWCandidate1_E").set(*event, resolvedWCandidates.candidate1.E(), sys);

            m_Fbranches.at("resolvedWCandidate2_m").set(*event, resolvedWCandidates.candidate2.M(), sys);
            m_Fbranches.at("resolvedWCandidate2_pt").set(*event, resolvedWCandidates.candidate2.Pt(), sys);
            m_Fbranches.at("resolvedWCandidate2_eta").set(*event, resolvedWCandidates.candidate2.Eta(), sys);
            m_Fbranches.at("resolvedWCandidate2_phi").set(*event, resolvedWCandidates.candidate2.Phi(), sys);
            m_Fbranches.at("resolvedWCandidate2_E").set(*event, resolvedWCandidates.candidate2.E(), sys);
          }

          if (resolvedZCandidates.valid){
            m_Fbranches.at("resolvedZCandidate1_m").set(*event, resolvedZCandidates.candidate1.M(), sys);
            m_Fbranches.at("resolvedZCandidate1_pt").set(*event, resolvedZCandidates.candidate1.Pt(), sys);
            m_Fbranches.at("resolvedZCandidate1_eta").set(*event, resolvedZCandidates.candidate1.Eta(), sys);
            m_Fbranches.at("resolvedZCandidate1_phi").set(*event, resolvedZCandidates.candidate1.Phi(), sys);
            m_Fbranches.at("resolvedZCandidate2_m").set(*event, resolvedZCandidates.candidate2.M(), sys);
            m_Fbranches.at("resolvedZCandidate2_pt").set(*event, resolvedZCandidates.candidate2.Pt(), sys);
            m_Fbranches.at("resolvedZCandidate2_eta").set(*event, resolvedZCandidates.candidate2.Eta(), sys);
            m_Fbranches.at("resolvedZCandidate2_phi").set(*event, resolvedZCandidates.candidate2.Phi(), sys);
            m_Fbranches.at("resolvedZCandidate2_E").set(*event, resolvedZCandidates.candidate2.E(), sys);
          }

    // resolvedVCandidates_resolvedH. 
    // After matching the VBS jets and selecting Higgs candidates. The resolved V jets are matched from the remaining non b-tagged jets.

        /////// Resolved H  Resolved V jets/////////
      auto potentialVCandidates = std::make_unique<ConstDataVector<xAOD::JetContainer>>(
              signalJets->begin(), signalJets->end(), SG::VIEW_ELEMENTS);
      // Erase HCandidates from signal jet container
      if (HJets->size() >=2){
        potentialVCandidates->erase( std::remove_if( potentialVCandidates->begin(), potentialVCandidates->end(),
                  [&](const xAOD::Jet* jet) -> bool { return jet == HJets->at(0) || jet == HJets->at(1) ;} ),
                  potentialVCandidates->end() );
      }

      auto VJets_resolvedH = std::make_unique<ConstDataVector<xAOD::JetContainer>> (SG::VIEW_ELEMENTS);
      for(const xAOD::Jet* jet : *potentialVCandidates) {
        // only central jets
        if (std::abs(jet->eta())>=2.5) continue;
        // non b-tagged jets 
        if ( !m_isBtag.empty() && m_isBtag.get(*jet, sys) ) continue;
        VJets_resolvedH->push_back(jet);
      }

      if (VJets_resolvedH->size() >=4){ // Both Vector bosons are resolved. 
        // convert to the form expected by findBestBosonPairing
        std::vector<const xAOD::Jet*> vjets;
        vjets.reserve(VJets_resolvedH->size());
        for (const xAOD::Jet* j : *VJets_resolvedH) vjets.push_back(j);

        // find best W pairing
        auto fullyresolvedWpair = findBestBosonPairing(vjets, Wmass);
        if (fullyresolvedWpair.valid) {
          m_Fbranches.at("fullyresolved_WCandidate1_m").set(*event, fullyresolvedWpair.candidate1.M(), sys);
          m_Fbranches.at("fullyresolved_WCandidate1_pt").set(*event, fullyresolvedWpair.candidate1.Pt(), sys);
          m_Fbranches.at("fullyresolved_WCandidate1_eta").set(*event, fullyresolvedWpair.candidate1.Eta(), sys);
          m_Fbranches.at("fullyresolved_WCandidate1_phi").set(*event, fullyresolvedWpair.candidate1.Phi(), sys);
          m_Fbranches.at("fullyresolved_WCandidate1_E").set(*event, fullyresolvedWpair.candidate1.E(), sys);

          m_Fbranches.at("fullyresolved_WCandidate2_m").set(*event, fullyresolvedWpair.candidate2.M(), sys);
          m_Fbranches.at("fullyresolved_WCandidate2_pt").set(*event, fullyresolvedWpair.candidate2.Pt(), sys);
          m_Fbranches.at("fullyresolved_WCandidate2_eta").set(*event, fullyresolvedWpair.candidate2.Eta(), sys);
          m_Fbranches.at("fullyresolved_WCandidate2_phi").set(*event, fullyresolvedWpair.candidate2.Phi(), sys);
          m_Fbranches.at("fullyresolved_WCandidate2_E").set(*event, fullyresolvedWpair.candidate2.E(), sys);
        }

        // find best Z pairing
        auto fullyresolvedZpair = findBestBosonPairing(vjets, Zmass);
        if (fullyresolvedZpair.valid) {
          m_Fbranches.at("fullyresolved_ZCandidate1_m").set(*event, fullyresolvedZpair.candidate1.M(), sys);
          m_Fbranches.at("fullyresolved_ZCandidate1_pt").set(*event, fullyresolvedZpair.candidate1.Pt(), sys);
          m_Fbranches.at("fullyresolved_ZCandidate1_eta").set(*event, fullyresolvedZpair.candidate1.Eta(), sys);
          m_Fbranches.at("fullyresolved_ZCandidate1_phi").set(*event, fullyresolvedZpair.candidate1.Phi(), sys);
          m_Fbranches.at("fullyresolved_ZCandidate1_E").set(*event, fullyresolvedZpair.candidate1.E(), sys);

          m_Fbranches.at("fullyresolved_ZCandidate2_m").set(*event, fullyresolvedZpair.candidate2.M(), sys);
          m_Fbranches.at("fullyresolved_ZCandidate2_pt").set(*event, fullyresolvedZpair.candidate2.Pt(), sys);
          m_Fbranches.at("fullyresolved_ZCandidate2_eta").set(*event, fullyresolvedZpair.candidate2.Eta(), sys);
          m_Fbranches.at("fullyresolved_ZCandidate2_phi").set(*event, fullyresolvedZpair.candidate2.Phi(), sys);
          m_Fbranches.at("fullyresolved_ZCandidate2_E").set(*event, fullyresolvedZpair.candidate2.E(), sys);
        }

      }
    } // if (m_doResolved)

      if(!m_UseVBFRNN){
            if ( vbsjets->size() >= 2 ){
              const xAOD::Jet* vbsJet1 = vbsjets->at(0);
              const xAOD::Jet* vbsJet2 = vbsjets->at(1);

              TLorentzVector vbs_jj = vbsJet1->p4() + vbsJet2->p4();
              
              m_Fbranches.at("VBSJ1_m").set(*event, vbsJet1->m(), sys);
              m_Fbranches.at("VBSJ1_pt").set(*event, vbsJet1->pt(), sys);
              m_Fbranches.at("VBSJ1_eta").set(*event, vbsJet1->eta(), sys);
              m_Fbranches.at("VBSJ1_phi").set(*event, vbsJet1->phi(), sys);

              m_Fbranches.at("VBSJ2_m").set(*event, vbsJet2->m(), sys);
              m_Fbranches.at("VBSJ2_pt").set(*event, vbsJet2->pt(), sys);
              m_Fbranches.at("VBSJ2_eta").set(*event, vbsJet2->eta(), sys);
              m_Fbranches.at("VBSJ2_phi").set(*event, vbsJet2->phi(), sys);

              m_Fbranches.at("VBSdijet_m").set(*event, vbs_jj.M(), sys);
              m_Fbranches.at("VBSdijet_pt").set(*event, vbs_jj.Pt(), sys);
              m_Fbranches.at("VBSdijet_eta").set(*event, vbs_jj.Eta(), sys);
              m_Fbranches.at("VBSdijet_phi").set(*event, vbs_jj.Phi(), sys);
              m_Fbranches.at("dRVBSjj").set(*event, (vbsJet1->p4()).DeltaR(vbsJet2->p4()), sys);
              m_Fbranches.at("dEtaVBSjj").set(*event, (vbsJet1->eta())-vbsJet2->eta(), sys);
              m_Fbranches.at("dPhiVBSjj").set(*event, (vbsJet1->p4()).DeltaPhi(vbsJet2->p4()), sys);
              
            }
          }

          //kinematics of RNN jets
          else {
            std::vector<const xAOD::JetContainer*> RNNJets = {RNNJets_boosted};
            std::vector<std::string> RNNJets_names = {"RNNJets_boostedCategory"};
            if (m_doResolved){
              RNNJets.emplace_back(RNNJets_resolved);
              RNNJets_names.emplace_back("RNNJets_resolvedCategory");
            }
            for(unsigned int i=0; i<RNNJets.size(); i++) {
              const xAOD::JetContainer *RNNJets_container = RNNJets[i];
              const std::string& RNNJets_container_name = RNNJets_names[i];

              for(unsigned int j=0; j<std::min(size_t(2),RNNJets_container->size()); j++) {
                std::string prefix = "Jet"+std::to_string(j+1);
                const xAOD::Jet* RNNJet = RNNJets_container->at(j);

                m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_m").set(*event, RNNJet->m(), sys);
                m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_pt").set(*event, RNNJet->pt(), sys);
                m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_eta").set(*event, RNNJet->eta(), sys);
                m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_phi").set(*event, RNNJet->phi(), sys);
              }
            }
          }
      }

    return StatusCode::SUCCESS;
  }

}
