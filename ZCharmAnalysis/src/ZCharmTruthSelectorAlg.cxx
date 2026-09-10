/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZCharmTruthSelectorAlg.h"
#include <AthenaKernel/Units.h>
#include <AsgDataHandles/ReadDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandleKey.h>
#include <AthContainers/ConstDataVector.h>


namespace ZCC
{
  StatusCode ZCharmTruthSelectorAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     ZCharmTruthSelectorAlg      \n");
    ATH_MSG_INFO("*********************************\n");


    ATH_CHECK (m_truthjetsInKey.initialize());
    ATH_CHECK (m_truthelectronInKey.initialize());
    ATH_CHECK (m_truthmuonInKey.initialize());
    ATH_CHECK (m_eventInKey.initialize());   
    ATH_CHECK (m_passTruthCutsKey.initialize());
    ATH_CHECK (m_jetTruthFlavourKey.initialize());

    for (auto& [boolKey, boolName] : m_boolnames) {   
      m_bools.emplace(boolKey, false);
      std::string fullKey = m_eventInKey.key() + "." + boolName;
      SG::WriteDecorHandleKey<xAOD::EventInfo> handleKey(fullKey);
      ATH_CHECK(handleKey.initialize());
      m_BbranchKeys.emplace(boolKey, handleKey);
      ATH_CHECK(m_BbranchKeys.at(boolKey).initialize());
    }   
    

    for (const std::string &var : m_floatVariables) {
      std::string varkey = m_eventInKey.key() + "." + var;
      SG::WriteDecorHandleKey<xAOD::EventInfo> floatVarKey(varkey);
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


  StatusCode ZCharmTruthSelectorAlg::execute()
  {
    m_bools.at(ZCC::IS_ee_TRUTH) = false;
    m_bools.at(ZCC::IS_mm_TRUTH) = false;
    m_bools.at(ZCC::IS_em_TRUTH) = false;

    m_bools.at(ZCC::EXACTLY_TWO_LEPTONS_TRUTH) = false;
    m_bools.at(ZCC::OPPOSITE_CHARGE_LEPTONS_TRUTH) = false;
    m_bools.at(ZCC::DILEPTON_MASS_WINDOW_TRUTH) = false;

    m_bools.at(ZCC::ONE_C_JETS_TRUTH) = false;
    m_bools.at(ZCC::TWO_C_JETS_TRUTH) = false;

    // FilterReporter filter(m_filterParams, false);

    SG::ReadHandle<xAOD::EventInfo> truthEvents(m_eventInKey);
    SG::ReadHandle<xAOD::JetContainer> truthJets(m_truthjetsInKey);
    SG::ReadHandle<xAOD::TruthParticleContainer> truthElectrons(m_truthelectronInKey);
    SG::ReadHandle<xAOD::TruthParticleContainer> truthMuons(m_truthmuonInKey);
    SG::ReadDecorHandle<xAOD::JetContainer, int> truthFlavour(m_jetTruthFlavourKey);
    auto jets_after_overlap = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
    auto truthcjets = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);

    for (auto [var, key] : m_FbranchesKeys){
      SG::WriteDecorHandle<xAOD::EventInfo, float> floatDecorator(key);
      m_Fbranches.insert_or_assign(var, std::move(floatDecorator));
    }

    const xAOD::EventInfo* truthevent = truthEvents.get();

    for (const std::string &string_var: m_floatVariables) {
      m_Fbranches.at(string_var)(*truthevent) = -99.;
    }

    auto muons_after_selection = std::make_unique<ConstDataVector<xAOD::TruthParticleContainer>>(SG::VIEW_ELEMENTS);
    auto electrons_after_selection = std::make_unique<ConstDataVector<xAOD::TruthParticleContainer>>(SG::VIEW_ELEMENTS);

    evaluateTruthLeptonCuts(*truthevent, *truthElectrons, *truthMuons,
			    *electrons_after_selection, *muons_after_selection);

    truthOverlapRemoval(*truthJets, *electrons_after_selection,
			*muons_after_selection, *jets_after_overlap);

    for (const xAOD::Jet *jet : *jets_after_overlap){
      if (truthFlavour(*jet) == 4) truthcjets->push_back(jet);
    }

    int n_jets = jets_after_overlap->size();
    for (int i=0; i<std::min(n_jets, 2); ++i) {
      std::string prefix = "TruthJet_"+std::to_string(i+1);
      m_Fbranches.at(prefix+"_pt")(*truthevent) = jets_after_overlap->at(i)->pt();
      m_Fbranches.at(prefix+"_eta")(*truthevent) = jets_after_overlap->at(i)->eta();
      m_Fbranches.at(prefix+"_phi")(*truthevent) = jets_after_overlap->at(i)->phi();
      m_Fbranches.at(prefix+"_E")(*truthevent) = jets_after_overlap->at(i)->e();
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

    for (const xAOD::TruthParticle* electron : *electrons_after_selection){
      if (!ele0) ele0 = electron;
      else {
        ele1 = electron;
        break;
      }
    }

    const xAOD::TruthParticle* mu0 = nullptr;
    const xAOD::TruthParticle* mu1 = nullptr;

    for (const xAOD::TruthParticle* muon : *muons_after_selection){
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
          SG::ConstAccessor<float> dressedLeptonPt("pt_dressed");
          return dressedLeptonPt(*a) > dressedLeptonPt(*b); });

    SG::ConstAccessor<float> dressedLeptonPt("pt_dressed");
    SG::ConstAccessor<float> dressedLeptonEta("eta_dressed");
    SG::ConstAccessor<float> dressedLeptonPhi("phi_dressed");
    SG::ConstAccessor<float> dressedLeptonE("e_dressed");
    SG::ConstAccessor<float> leptonM("m");

    int n_leptons = leptons.size();
    for (int i = 0; i < std::min(n_leptons, 2); ++i){
      std::string prefix = "TruthLepton_"+std::to_string(i+1);
      m_Fbranches.at(prefix+"_pt")(*truthevent) = dressedLeptonPt(*leptons.at(i));
      m_Fbranches.at(prefix+"_eta")(*truthevent) = dressedLeptonEta(*leptons.at(i));
      m_Fbranches.at(prefix+"_phi")(*truthevent) = dressedLeptonPhi(*leptons.at(i));
      m_Fbranches.at(prefix+"_E")(*truthevent) = dressedLeptonE(*leptons.at(i));
      m_Fbranches.at(prefix+"_m")(*truthevent) = leptonM(*leptons.at(i));
    }

    evaluateTruthCJetCuts(*truthcjets);
    
    char pass_truth_baseline=true;
    for (const auto& [key, value] : m_boolnames) {
      auto it = std::find(m_BASELINE_CUTS.begin(), m_BASELINE_CUTS.end(), value);
      if (it != m_BASELINE_CUTS.end()) {
        char cut_passed = m_bools.at(key);
        pass_truth_baseline = pass_truth_baseline && cut_passed;
      }
    }

    if (m_saveCutFlow){
      std::vector<bool> cutBools;
      for(const auto& cut : m_inputCutList)
	cutBools.push_back(m_bools.at(m_nameToBool.at(cut)));
      ATH_CHECK (applyCutFlow(cutBools));
    }

    SG::WriteDecorHandle<xAOD::EventInfo, char> passTruthCutsHandle(m_passTruthCutsKey);
    passTruthCutsHandle(*truthevent) = pass_truth_baseline && jets_after_overlap->size()>=1;

    for (auto& [key, handleKey] : m_BbranchKeys) {
      SG::WriteDecorHandle<xAOD::EventInfo, char> handle(handleKey);
      handle(*truthevent) = m_bools.at(key);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZCharmTruthSelectorAlg::finalize()
  {
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);
    ATH_CHECK (finaliseCutflow());
    return StatusCode::SUCCESS;
  }


  void ZCharmTruthSelectorAlg::truthOverlapRemoval
  (const xAOD::JetContainer& truthJets,
   ConstDataVector<xAOD::TruthParticleContainer>& truthElectrons,
   ConstDataVector<xAOD::TruthParticleContainer>& truthMuons,
   ConstDataVector<xAOD::JetContainer>& jetsAfterOverlap){

    float dRcut = 0.4;
    SG::ConstAccessor<float> dressedLeptonPt("pt_dressed");
    SG::ConstAccessor<float> dressedLeptonEta("eta_dressed");
    SG::ConstAccessor<float> dressedLeptonPhi("phi_dressed");
    SG::ConstAccessor<float> dressedLeptonE("e_dressed");
    for (const xAOD::Jet* jet : truthJets){
      TLorentzVector jet_p4 = jet->p4();
      bool keep = true;
      for (const xAOD::TruthParticle* ele : truthElectrons){
        TLorentzVector ele_p4;
        ele_p4.SetPtEtaPhiE(dressedLeptonPt(*ele),
                            dressedLeptonEta(*ele),
                            dressedLeptonPhi(*ele),
                            dressedLeptonE(*ele));
        if (jet_p4.DeltaR(ele_p4, true) < dRcut){
          keep = false;
          break;
        }
      }
      if (!keep) continue;
      for (const xAOD::TruthParticle* mu : truthMuons){
        TLorentzVector mu_p4;
        mu_p4.SetPtEtaPhiE(dressedLeptonPt(*mu),
                           dressedLeptonEta(*mu),
                           dressedLeptonPhi(*mu),
                           dressedLeptonE(*mu));
        if (jet_p4.DeltaR(mu_p4, true) < dRcut){
          keep = false;
          break;
        }
      }
      if (keep){
        jetsAfterOverlap.push_back(jet);
      }
    }

  }

  void ZCharmTruthSelectorAlg::evaluateTruthLeptonCuts
  (const xAOD::EventInfo& truthevent,
   const xAOD::TruthParticleContainer& truthelectrons,
   const xAOD::TruthParticleContainer& truthmuons,
   ConstDataVector<xAOD::TruthParticleContainer>& electronsAfterSelection,
   ConstDataVector<xAOD::TruthParticleContainer>& muonsAfterSelection)
  {
    TLorentzVector ll;

    bool OPPOSITE_CHARGE_LEPTONS_TRUTH = false;

    SG::ConstAccessor<unsigned int> classifierParticleType("classifierParticleType");
    SG::ConstAccessor<unsigned int> classifierParticleOrigin("classifierParticleOrigin");
    SG::ConstAccessor<float> dressedLeptonPt("pt_dressed");
    SG::ConstAccessor<float> dressedLeptonEta("eta_dressed");
    SG::ConstAccessor<float> dressedLeptonPhi("phi_dressed");
    SG::ConstAccessor<float> dressedLeptonE("e_dressed");
    
    for (const xAOD::TruthParticle* electron : truthelectrons){
      unsigned int ele_type = classifierParticleType(*electron);
      unsigned int ele_origin = classifierParticleOrigin(*electron);
      bool electron_fromZ = ((ele_type >= 1 && ele_type <= 4) && (ele_origin == 13));
      if (!electron_fromZ || dressedLeptonPt(*electron) < 25.*Athena::Units::GeV || std::abs(dressedLeptonEta(*electron)) > 2.5) continue;
      electronsAfterSelection.push_back(electron);
    }

    for (const xAOD::TruthParticle* muon : truthmuons){
      unsigned int mu_type = classifierParticleType(*muon);
      unsigned int mu_origin = classifierParticleOrigin(*muon);
      bool muon_fromZ = ((mu_type >= 5 && mu_type <= 8) && (mu_origin == 13));
      if (!muon_fromZ || dressedLeptonPt(*muon) < 25.*Athena::Units::GeV || std::abs(dressedLeptonEta(*muon)) > 2.5) continue;
      muonsAfterSelection.push_back(muon);
    }

    if (electronsAfterSelection.size() + muonsAfterSelection.size() == 2) {
      
      m_bools.at(ZCC::EXACTLY_TWO_LEPTONS_TRUTH) = true;
    }
      
    if (electronsAfterSelection.size() >= 2 && muonsAfterSelection.size() == 0)
    {
      m_bools.at(ZCC::IS_ee_TRUTH) = true;
      TLorentzVector ele0;
      ele0.SetPtEtaPhiE(dressedLeptonPt(*electronsAfterSelection.at(0)),
                        dressedLeptonEta(*electronsAfterSelection.at(0)),
                        dressedLeptonPhi(*electronsAfterSelection.at(0)),
                        dressedLeptonE(*electronsAfterSelection.at(0)));
      TLorentzVector ele1;
      ele1.SetPtEtaPhiE(dressedLeptonPt(*electronsAfterSelection.at(1)),
                        dressedLeptonEta(*electronsAfterSelection.at(1)),
                        dressedLeptonPhi(*electronsAfterSelection.at(1)),
                        dressedLeptonE(*electronsAfterSelection.at(1)));
      ll = ele0 + ele1;
      OPPOSITE_CHARGE_LEPTONS_TRUTH = electronsAfterSelection.at(0)->charge()*electronsAfterSelection.at(1)->charge() == -1;
    }

    if (muonsAfterSelection.size() >= 2 && electronsAfterSelection.size() == 0)
    {
      m_bools.at(ZCC::IS_mm_TRUTH) = true;
      TLorentzVector mu0;
      mu0.SetPtEtaPhiE(dressedLeptonPt(*muonsAfterSelection.at(0)),
                       dressedLeptonEta(*muonsAfterSelection.at(0)),
                       dressedLeptonPhi(*muonsAfterSelection.at(0)),
                       dressedLeptonE(*muonsAfterSelection.at(0)));
      TLorentzVector mu1;
      mu1.SetPtEtaPhiE(dressedLeptonPt(*muonsAfterSelection.at(1)),
                       dressedLeptonEta(*muonsAfterSelection.at(1)),
                       dressedLeptonPhi(*muonsAfterSelection.at(1)),
                       dressedLeptonE(*muonsAfterSelection.at(1)));
      ll = mu0 + mu1;
      OPPOSITE_CHARGE_LEPTONS_TRUTH = muonsAfterSelection.at(0)->charge()*muonsAfterSelection.at(1)->charge() == -1;
    }

    if (electronsAfterSelection.size() == 1 && muonsAfterSelection.size() == 1)
    {
      m_bools.at(ZCC::IS_em_TRUTH) = true;
      TLorentzVector ele0;
      ele0.SetPtEtaPhiE(dressedLeptonPt(*electronsAfterSelection.at(0)),
                        dressedLeptonEta(*electronsAfterSelection.at(0)),
                        dressedLeptonPhi(*electronsAfterSelection.at(0)),
                        dressedLeptonE(*electronsAfterSelection.at(0)));
      TLorentzVector mu0;
      mu0.SetPtEtaPhiE(dressedLeptonPt(*muonsAfterSelection.at(0)),
                       dressedLeptonEta(*muonsAfterSelection.at(0)),
                       dressedLeptonPhi(*muonsAfterSelection.at(0)),
                       dressedLeptonE(*muonsAfterSelection.at(0)));
      ll = ele0 + mu0;
      OPPOSITE_CHARGE_LEPTONS_TRUTH = electronsAfterSelection.at(0)->charge()*muonsAfterSelection.at(0)->charge() == -1;
    }
    
    double mll = ll.M();

    std::string prefix = "Truth_ll";
    m_Fbranches.at(prefix+"_pt")(truthevent) = ll.Pt();
    m_Fbranches.at(prefix+"_eta")(truthevent) = ll.Eta();
    m_Fbranches.at(prefix+"_phi")(truthevent) = ll.Phi();
    m_Fbranches.at(prefix+"_E")(truthevent) = ll.E();
    m_Fbranches.at(prefix+"_m")(truthevent) = mll;

    m_bools.at(ZCC::OPPOSITE_CHARGE_LEPTONS_TRUTH) = OPPOSITE_CHARGE_LEPTONS_TRUTH;
    m_bools.at(ZCC::DILEPTON_MASS_WINDOW_TRUTH) = (mll >= 76.*Athena::Units::GeV && mll <= 106.*Athena::Units::GeV );
  } 
  
  void ZCharmTruthSelectorAlg::evaluateTruthCJetCuts
  (const ConstDataVector<xAOD::JetContainer>& truthcjets)
  { 
    ///All jets in the containers should have pT>20GeV. Check minPt of your JetSelectorAlg in the ZCharm_config file.
    m_bools.at(ZCC::ONE_C_JETS_TRUTH) = (truthcjets.size() >= 1);
    m_bools.at(ZCC::TWO_C_JETS_TRUTH) = (truthcjets.size() >= 2);
  }

}
