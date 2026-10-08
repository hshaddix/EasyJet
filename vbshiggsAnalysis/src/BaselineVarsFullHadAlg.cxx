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


namespace VBSHIGGS {
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

          
    if( m_UseVBFRNN ){
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

  StatusCode BaselineVarsFullHadAlg::execute(const EventContext& ctx) {

    for (const auto& sys : m_systematicsList.systematicsVector()) {

      const xAOD::EventInfo* event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::JetContainer* largeJets = nullptr;
      ANA_CHECK(m_vbsLRJetHandle.retrieve(largeJets, sys, ctx));

      const xAOD::JetContainer *signalJets = nullptr;    
      const xAOD::JetContainer *HJets = nullptr;
      if (m_doResolved){
        ANA_CHECK (m_signaljetHandle.retrieve (signalJets, sys, ctx));
        ANA_CHECK (m_HCandHandle.retrieve (HJets, sys, ctx));
      }

      
  
      const xAOD::JetContainer *RNNJets_boosted= nullptr;
      const xAOD::JetContainer *RNNJets_resolved = nullptr;
      if(m_UseVBFRNN) {
        ANA_CHECK (m_RNNjetBoostedHandle.retrieve (RNNJets_boosted, sys, ctx));
        if (m_doResolved)
	  ANA_CHECK (m_RNNjetResolvedHandle.retrieve (RNNJets_resolved, sys, ctx));
      }



      for (const auto& v : m_floatVariables)
        m_Fbranches.at(v).set(*event, -99.f, sys);
      for (const auto& v : m_intVariables)
        m_Ibranches.at(v).set(*event, -99, sys);

    
      int nLargeJets   = largeJets->size();
      bool WPgiven = !m_isBtag.empty();
      if (m_doResolved) {

    int n_jets = signalJets->size();
    

    auto bjets = std::make_unique<
        ConstDataVector<xAOD::JetContainer>
    >(SG::VIEW_ELEMENTS);

    int nBJets = 0;

    for (const xAOD::Jet* jet : *signalJets) {

  

      if (WPgiven &&
          std::abs(jet->eta()) < 2.5 &&
          m_isBtag.get(*jet, sys)) {

        bjets->push_back(jet);
        nBJets++;
      }
    }


    m_Ibranches.at("nJets").set(*event, n_jets, sys);
    m_Ibranches.at("nBJets").set(*event, nBJets, sys);
    m_Ibranches.at("nLargeJets").set(*event, nLargeJets, sys);
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

  

   
      }

    return StatusCode::SUCCESS;
  }

}
