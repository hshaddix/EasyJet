/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZQGTruthSelectorAlg.h"
#include <AthenaKernel/Units.h>
#include <AsgDataHandles/ReadDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandleKey.h>
#include <AthContainers/ConstDataVector.h>


namespace ZQG
{
  StatusCode ZQGTruthSelectorAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     ZQGTruthSelectorAlg      \n");
    ATH_MSG_INFO("*********************************\n");


    ATH_CHECK (m_truthjetsInKey.initialize());
    ATH_CHECK (m_truthlargejetInKey.initialize());
    ATH_CHECK (m_truthelectronInKey.initialize());
    ATH_CHECK (m_truthmuonInKey.initialize());
    ATH_CHECK (m_eventInKey.initialize());   
    ATH_CHECK (m_passTruthCutsKey.initialize());
    ATH_CHECK (m_jetTruthFlavourKey.initialize());

    for (auto& [boolKey, boolName] : m_boolnames) {   
      m_bools.emplace(boolKey, false);
      std::string fullKey = m_eventInKey.key() + "." + boolName;
      SG::WriteDecorHandleKey<xAOD::TruthEventContainer> handleKey(fullKey);
      ATH_CHECK(handleKey.initialize());
      m_BbranchKeys.emplace(boolKey, handleKey);
      ATH_CHECK(m_BbranchKeys.at(boolKey).initialize());
    }   
    

    for (const std::string &var : m_floatVariables) {
      std::string varkey = m_eventInKey.key() + "." + var;
      SG::WriteDecorHandleKey<xAOD::TruthEventContainer> floatVarKey(varkey);
      m_FbranchesKeys.emplace(var, floatVarKey);
      ATH_CHECK (m_FbranchesKeys.at(var).initialize());
    }
    
    if(m_saveCutFlow){
      std::vector<std::string> boolNameList;
      for (const auto& [key, value]: m_boolnames){
        boolNameList.push_back(value);
        m_nameToBool[value] = key;
      }
      ATH_CHECK (initialiseCutflow(boolNameList, "Truth"));
    }

    return StatusCode::SUCCESS;

  }


  StatusCode ZQGTruthSelectorAlg::execute()
  {

    // FilterReporter filter(m_filterParams, false);

    SG::ReadHandle<xAOD::TruthEventContainer> truthEvents(m_eventInKey);
    SG::ReadHandle<xAOD::JetContainer> truthJets(m_truthjetsInKey);
    SG::ReadHandle<xAOD::JetContainer> truthLargeJets(m_truthlargejetInKey);
    SG::ReadHandle<xAOD::TruthParticleContainer> truthElectrons(m_truthelectronInKey);
    SG::ReadHandle<xAOD::TruthParticleContainer> truthMuons(m_truthmuonInKey);
    SG::ReadDecorHandle<xAOD::JetContainer, int> truthFlavour(m_jetTruthFlavourKey);
    auto jets_after_overlap = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
    auto truthbjets = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
    auto truthcjets = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);

    for (auto [var, key] : m_FbranchesKeys){
      SG::WriteDecorHandle<xAOD::TruthEventContainer, float> floatDecorator(key);
      m_Fbranches.emplace(var, floatDecorator);
    }

    const xAOD::TruthEvent* truthevent = truthEvents->at(0);

    for (const std::string &string_var: m_floatVariables) {
      m_Fbranches.at(string_var)(*truthevent) = -99.;
    }

    truthOverlapRemoval(*truthJets, *truthElectrons, *truthMuons, *jets_after_overlap);
    for (const xAOD::Jet *jet : *jets_after_overlap){
      int truthlabel = truthFlavour(*jet);
      if(truthlabel == 5) truthbjets->push_back(jet);
      if(truthlabel == 4) truthcjets->push_back(jet);
    }

    int n_cjets = truthcjets->size();
    for (int i=0; i<std::min(n_cjets, 2); ++i){
      std::string prefix = "TruthJet_c"+std::to_string(i+1);
      m_Fbranches.at(prefix+"_pt")(*truthevent) = truthcjets->at(i)->pt();
      m_Fbranches.at(prefix+"_eta")(*truthevent) = truthcjets->at(i)->eta();
      m_Fbranches.at(prefix+"_phi")(*truthevent) = truthcjets->at(i)->phi();
      m_Fbranches.at(prefix+"_E")(*truthevent) = truthcjets->at(i)->e();
    }

    // Leptons
    const xAOD::TruthParticle* ele0 = nullptr;
    const xAOD::TruthParticle* ele1 = nullptr;

    for (const xAOD::TruthParticle* electron : *truthElectrons){
      if (!ele0) ele0 = electron;
      else {
        ele1 = electron;
        break;
      }
    }

    const xAOD::TruthParticle* mu0 = nullptr;
    const xAOD::TruthParticle* mu1 = nullptr;

    for (const xAOD::TruthParticle* muon : *truthMuons){
      if (!mu0) mu0 = muon;
      else {
        mu1 = muon;
        break;
      }
    }

    std::vector<const xAOD::TruthParticle*> leptons;
    if(ele0) leptons.emplace_back(ele0);
    if(mu0) leptons.emplace_back(mu0);
    if(ele1) leptons.emplace_back(ele1);
    if(mu1) leptons.emplace_back(mu1);

    std::sort(leptons.begin(), leptons.end(),
    [](const xAOD::TruthParticle*& a,
        const xAOD::TruthParticle*& b) {
      return a->pt() > b->pt(); });


    m_bools.at(ZQG::IS_ee_TRUTH) = false;
    m_bools.at(ZQG::IS_mm_TRUTH) = false;
    m_bools.at(ZQG::IS_em_TRUTH) = false;

    m_bools.at(ZQG::EXACTLY_TWO_LEPTONS_TRUTH) = false;
    m_bools.at(ZQG::OPPOSITE_CHARGE_LEPTONS_TRUTH) = false;
    m_bools.at(ZQG::DILEPTON_MASS_WINDOW_TRUTH) = false;

    m_bools.at(ZQG::ONE_B_JETS_TRUTH) = false;
    m_bools.at(ZQG::TWO_B_JETS_TRUTH) = false;
    m_bools.at(ZQG::ONE_C_JETS_TRUTH) = false;
    m_bools.at(ZQG::TWO_C_JETS_TRUTH) = false;
    m_bools.at(ZQG::ONE_LARGE_JET_TRUTH) = false;

    evaluateTruthLeptonCuts(*truthevent, *truthElectrons, *truthMuons);
    evaluateTruthBJetCuts(*truthbjets);
    evaluateTruthCJetCuts(*truthcjets);
    evaluateTruthLargeJetCuts(*truthLargeJets);
    
    bool pass_truth_baseline=true;
    for (const auto& [key, value] : m_boolnames) {
      auto it = std::find(m_BASELINE_CUTS.begin(), m_BASELINE_CUTS.end(), value);
      if (it != m_BASELINE_CUTS.end()) {
        pass_truth_baseline &= m_bools.at(key);
      }
    }

    if (m_saveCutFlow){
      std::vector<bool> cutBools;
      for(const auto& cut : m_inputCutList)
	cutBools.push_back(m_bools.at(m_nameToBool.at(cut)));
      ATH_CHECK (applyCutFlow(cutBools));
    }

    ATH_MSG_VERBOSE("pass_truth_baseline = " << pass_truth_baseline);
    SG::WriteDecorHandle<xAOD::TruthEventContainer, char> passTruthCutsHandle(m_passTruthCutsKey);
    passTruthCutsHandle(*truthevent) = pass_truth_baseline;

    for (auto& [key, handleKey] : m_BbranchKeys) {
      SG::WriteDecorHandle<xAOD::TruthEventContainer, char> handle(handleKey);
      handle(*truthevent) = m_bools.at(key);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZQGTruthSelectorAlg::finalize()
  {
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);
    ATH_CHECK (finaliseCutflow());
    return StatusCode::SUCCESS;
  }

  void ZQGTruthSelectorAlg::truthOverlapRemoval
  (const xAOD::JetContainer& truthJets,
   const xAOD::TruthParticleContainer& truthElectrons,
   const xAOD::TruthParticleContainer& truthMuons,
   ConstDataVector<xAOD::JetContainer>& jetsAfterOverlap){

    float dRcut = 0.4;
    for (const xAOD::Jet* jet : truthJets){
      TLorentzVector jet_p4 = jet->p4();
      bool keep = true;
      for (const xAOD::TruthParticle* ele : truthElectrons){
        TLorentzVector ele_p4 = ele->p4();
        if (jet_p4.DeltaR(ele_p4) < dRcut){
          keep = false;
          break;
        }
      }
      if (!keep) continue;
      for (const xAOD::TruthParticle* mu : truthMuons){
        TLorentzVector mu_p4 = mu->p4();
        if (jet_p4.DeltaR(mu_p4) < dRcut){
          keep = false;
          break;
        }
      }
      if (keep){
        jetsAfterOverlap.push_back(jet);
      }
    }

  }

  void ZQGTruthSelectorAlg::evaluateTruthLeptonCuts
  (const xAOD::TruthEvent& truthevent,
   const xAOD::TruthParticleContainer& truthelectrons,
   const xAOD::TruthParticleContainer& truthmuons)
  {
  
    TLorentzVector ll;

    bool OPPOSITE_CHARGE_LEPTONS_TRUTH = false;

    if (truthelectrons.size() + truthmuons.size() == 2) {
    
      m_bools.at(ZQG::EXACTLY_TWO_LEPTONS_TRUTH) = true;

    }
      
    if (truthelectrons.size() >= 2 && truthmuons.size() == 0)
    {
      m_bools.at(ZQG::IS_ee_TRUTH) = true;
      ll = truthelectrons.at(0)->p4() + truthelectrons.at(1)->p4();
      OPPOSITE_CHARGE_LEPTONS_TRUTH = truthelectrons.at(0)->charge()*truthelectrons.at(1)->charge() == -1;
    }

    if (truthmuons.size() >= 2 && truthelectrons.size() == 0)
    {
      m_bools.at(ZQG::IS_mm_TRUTH) = true;
      ll = truthmuons.at(0)->p4() + truthmuons.at(1)->p4();
      OPPOSITE_CHARGE_LEPTONS_TRUTH = truthmuons.at(0)->charge()*truthmuons.at(1)->charge() == -1;
    }

    if (truthelectrons.size() == 1 && truthmuons.size() == 1)
    {
      m_bools.at(ZQG::IS_em_TRUTH) = true;
      ll = truthelectrons.at(0)->p4() + truthmuons.at(0)->p4();
      OPPOSITE_CHARGE_LEPTONS_TRUTH = truthelectrons.at(0)->charge()*truthmuons.at(0)->charge() == -1;
    }
    

    double mll = ll.M();

    std::string prefix = "Truth_ll";
    m_Fbranches.at(prefix+"_pt")(truthevent) = ll.Pt();
    m_Fbranches.at(prefix+"_eta")(truthevent) = ll.Eta();
    m_Fbranches.at(prefix+"_phi")(truthevent) = ll.Phi();
    m_Fbranches.at(prefix+"_E")(truthevent) = ll.E();
    m_Fbranches.at(prefix+"_m")(truthevent) = mll;

    m_bools.at(ZQG::OPPOSITE_CHARGE_LEPTONS_TRUTH) = OPPOSITE_CHARGE_LEPTONS_TRUTH;
    m_bools.at(ZQG::DILEPTON_MASS_WINDOW_TRUTH) = ( mll >= 76.*Athena::Units::GeV && mll <= 106.*Athena::Units::GeV );

  }

  void ZQGTruthSelectorAlg::evaluateTruthBJetCuts
  (const ConstDataVector<xAOD::JetContainer>& truthbjets)
  { 
    ///All jets in the containers should have pT>20GeV. Check minPt of your JetSelectorAlg in the ZQG_config file.
    m_bools.at(ZQG::ONE_B_JETS_TRUTH) = truthbjets.size() >= 1;
    m_bools.at(ZQG::TWO_B_JETS_TRUTH) = truthbjets.size() >= 2;
  }  
  
  void ZQGTruthSelectorAlg::evaluateTruthCJetCuts
  (const ConstDataVector<xAOD::JetContainer>& truthcjets)
  { 
    ///All jets in the containers should have pT>20GeV. Check minPt of your JetSelectorAlg in the ZQG_config file.
    m_bools.at(ZQG::ONE_C_JETS_TRUTH) = truthcjets.size() >= 1;
    m_bools.at(ZQG::TWO_C_JETS_TRUTH) = truthcjets.size() >= 2;
  }  

  void ZQGTruthSelectorAlg::evaluateTruthLargeJetCuts
  (const xAOD::JetContainer& truthlargeJets)
  {
    m_bools.at(ZQG::ONE_LARGE_JET_TRUTH) = (truthlargeJets.size() >= 1);
  }

}

