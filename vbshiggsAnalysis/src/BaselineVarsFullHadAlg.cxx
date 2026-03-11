/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "BaselineVarsFullHadAlg.h"
#include "AthContainers/AuxElement.h"
#include "TLorentzVector.h"
#include <cmath>
#include "AthContainers/ConstDataVector.h"

namespace VBSHIGGS {

BaselineVarsFullHadAlg::BaselineVarsFullHadAlg(
  const std::string& name, ISvcLocator* pSvcLocator)
  : AthHistogramAlgorithm(name, pSvcLocator) {}

StatusCode BaselineVarsFullHadAlg::initialize() {

  ATH_MSG_INFO("*********************************");
  ATH_MSG_INFO("       FullHadBaselineVarsAlg    ");
  ATH_MSG_INFO("*********************************");

  // if (m_doResolved){
  //       ATH_CHECK (m_signaljetHandle.initialize(m_systematicsList));
  //       ATH_CHECK (m_HCandHandle.initialize(m_systematicsList));
  //     }

  ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
  ATH_CHECK(m_metHandle.initialize(m_systematicsList));
  ATH_CHECK(m_vbsLRJetHandle.initialize(m_systematicsList));
  // ATH_CHECK(m_vbsjetHandle.initialize(m_systematicsList));
  ATH_CHECK (m_vbsElectronHandle.initialize(m_systematicsList));
  ATH_CHECK (m_vbsMuonHandle.initialize(m_systematicsList));

  
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
  ATH_CHECK(m_systematicsList.initialize());
  return StatusCode::SUCCESS;
}

StatusCode BaselineVarsFullHadAlg::execute() {

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

    const xAOD::MuonContainer *muons = nullptr;
    ANA_CHECK (m_vbsMuonHandle.retrieve (muons, sys));
    
    const xAOD::ElectronContainer *electrons = nullptr;
    ANA_CHECK (m_vbsElectronHandle.retrieve (electrons, sys));


    
    const xAOD::MissingETContainer* metCont = nullptr;
    ANA_CHECK(m_metHandle.retrieve(metCont, sys));
    const xAOD::MissingET* met = (*metCont)["Final"];
    if (!met) return StatusCode::FAILURE;

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

  
      if (m_doResolved){
          //b-jet sector
          for (std::size_t i=0; i<std::min(HJets->size(),(std::size_t)2); i++){
            std::string prefix = "Jet_Higgs_candidate"+std::to_string(i+1);
            m_Fbranches.at(prefix+"_m").set(*event, HJets->at(i)->m(), sys);
            m_Fbranches.at(prefix+"_pt").set(*event, HJets->at(i)->pt(), sys);
            m_Fbranches.at(prefix+"_eta").set(*event, HJets->at(i)->eta(), sys);
            m_Fbranches.at(prefix+"_phi").set(*event, HJets->at(i)->phi(), sys);
            m_Fbranches.at(prefix+"_E").set(*event, HJets->at(i)->e(), sys);

           }

          TLorentzVector HDijet;
          TLorentzVector HJet1;
          TLorentzVector HJet2;
          if (HJets->size() >=2){
            HDijet = HJets->at(0)->p4()+ HJets->at(1)->p4();
            HJet1 = HJets->at(0)->p4();
            HJet2 = HJets->at(1)->p4();
            m_Fbranches.at("Hdijet_m").set(*event, HDijet.M(), sys);
            m_Fbranches.at("Hdijet_pt").set(*event, HDijet.Pt(), sys);
            m_Fbranches.at("Hdijet_eta").set(*event, HDijet.Eta(), sys);
            m_Fbranches.at("Hdijet_phi").set(*event, HDijet.Phi(), sys);
            m_Fbranches.at("dRHjj").set(*event, HJet1.DeltaR(HJet2), sys);
            m_Fbranches.at("dPhiHjj").set(*event, HJet1.DeltaPhi(HJet2), sys);
            m_Fbranches.at("dEtaHjj").set(*event, HJet1.Eta() - HJet2.Eta(), sys);
          }



      if (HJets->size() >= 2) {
        TLorentzVector h1 = HJets->at(0)->p4();
        TLorentzVector h2 = HJets->at(1)->p4();
        TLorentzVector hh = h1 + h2;

        m_Fbranches.at("Hdijet_m").set(*event, hh.M(), sys);
        m_Fbranches.at("Hdijet_pt").set(*event, hh.Pt(), sys);
        m_Fbranches.at("dRHjj").set(*event, h1.DeltaR(h2), sys);
      }
      }
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
              const xAOD::Jet* RNNJet = RNNJets_container->at(0);

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
