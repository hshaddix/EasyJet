/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


#include "BaselineVarstwoLepAlg.h"
#include "AthContainers/AuxElement.h"
#include "TLorentzVector.h"

#include <AthenaKernel/Units.h>

namespace VBSSEMILEP{
    BaselineVarstwoLepAlg::BaselineVarstwoLepAlg(const std::string &name,
                                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator){

    }

    StatusCode BaselineVarstwoLepAlg::initialize(){
      ATH_MSG_INFO("*********************************\n");
      ATH_MSG_INFO("       twoLepBaselineVarsAlg    \n");
      ATH_MSG_INFO("*********************************\n");

      // Read syst-aware input handles
      ATH_CHECK (m_smalljetHandle.initialize(m_systematicsList));
      if (m_doResolved){
        ATH_CHECK (m_signaljetHandle.initialize(m_systematicsList));
      }
      ATH_CHECK (m_vbsLRJetHandle.initialize(m_systematicsList));
      ATH_CHECK (m_vbsElectronHandle.initialize(m_systematicsList));
      ATH_CHECK (m_vbsMuonHandle.initialize(m_systematicsList));
      ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

      if( !m_UseVBFRNN ){
        ATH_CHECK (m_vbsJetHandle.initialize(m_systematicsList));
      }
      else {
        ATH_CHECK (m_RNNjetBoostedHandle.initialize(m_systematicsList));
        if (m_doResolved) 
          ATH_CHECK (m_RNNjetResolvedHandle.initialize(m_systematicsList));
      }

      if(m_isMC){
        ATH_CHECK (m_ele_truthOrigin.initialize(m_systematicsList, m_vbsElectronHandle));
        ATH_CHECK (m_ele_truthType.initialize(m_systematicsList, m_vbsElectronHandle));
        ATH_CHECK (m_ele_firstEgMotherTruthType.initialize(m_systematicsList, m_vbsElectronHandle));
        ATH_CHECK (m_ele_firstEgMotherTruthOrigin.initialize(m_systematicsList, m_vbsElectronHandle));
        ATH_CHECK (m_ele_firstEgMotherPdgId.initialize(m_systematicsList, m_vbsElectronHandle));

        ATH_CHECK (m_mu_truthOrigin.initialize(m_systematicsList, m_vbsMuonHandle));
        ATH_CHECK (m_mu_truthType.initialize(m_systematicsList, m_vbsMuonHandle));

	ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
	m_ele_SF = CP::SysReadDecorHandle<float>("effSF_"+m_eleWPName+"_%SYS%", this);
	ATH_CHECK (m_ele_SF.initialize(m_systematicsList, m_electronHandle));

        ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
        m_mu_SF = CP::SysReadDecorHandle<float>("effSF_"+m_muWPName+"_%SYS%", this);
        ATH_CHECK (m_mu_SF.initialize(m_systematicsList, m_muonHandle));
      }
      
      if (!m_isBtag.empty()) {
      ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_smalljetHandle));
    }

      if (m_doResolved){
        if (!m_PCBT.empty()) {
        ATH_CHECK (m_PCBT.initialize(m_systematicsList, m_signaljetHandle));
      }
    }

      // Intialise syst-aware output decorators
      for (const std::string &var : m_floatVariables) {
        CP::SysWriteDecorHandle<float> whandle{var+"_%SYS%", this};
        m_Fbranches.emplace(var, whandle);
        ATH_CHECK (m_Fbranches.at(var).initialize(m_systematicsList, m_eventHandle));
      }

      for (const std::string &var : m_intVariables){
        ATH_MSG_DEBUG("initializing integer variable: " << var);
        CP::SysWriteDecorHandle<int> whandle{var+"_%SYS%", this};
        m_Ibranches.emplace(var, whandle);
        ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
      };

   /*
     for (const std::string &var : m_floatVariables) {
        m_Fbranches.emplace(var, CP::SysWriteDecorHandle<float>(var+"_%SYS%", this));
    }
    for (const std::string &var : m_intVariables) {
        m_Ibranches.emplace(var, CP::SysWriteDecorHandle<int>(var+"_%SYS%", this));
    }

    // Initialize all handles
    for (auto& [key, handle] : m_Fbranches) {
        ATH_CHECK(handle.initialize(m_systematicsList, m_eventHandle));
    }
    for (auto& [key, handle] : m_Ibranches) {
        ATH_CHECK(handle.initialize(m_systematicsList, m_eventHandle));
    }
    */
    
    if (m_isMC) {
        if (m_doResolved) {
            ATH_CHECK (m_truthFlav.initialize(m_systematicsList, m_signaljetHandle));
        }
        ATH_CHECK (m_truth_label.initialize(m_systematicsList, m_vbsLRJetHandle));
      }

      ATH_CHECK (m_eleECIDS.initialize(m_systematicsList, m_vbsElectronHandle));
      
      ATH_CHECK (m_GN3Xv01_pqcdcx.initialize(m_systematicsList, m_vbsLRJetHandle));
      ATH_CHECK (m_GN3Xv01_pWqq.initialize(m_systematicsList, m_vbsLRJetHandle));
      ATH_CHECK (m_GN3Xv01_pqcdbx.initialize(m_systematicsList, m_vbsLRJetHandle));
      ATH_CHECK (m_GN3Xv01_ptop.initialize(m_systematicsList, m_vbsLRJetHandle));
      
      m_WTag_score = CP::SysReadDecorHandle<float> (m_WTag_Type.value()+m_WTag_WP.value()+"Tagger_Score", this);
      m_Pass_WTag = CP::SysReadDecorHandle<char> (m_WTag_Type.value()+m_WTag_WP.value()+"Tagger_Tagged", this);
      ATH_CHECK(m_WTag_score.initialize(m_systematicsList, m_vbsLRJetHandle));
      ATH_CHECK(m_Pass_WTag.initialize(m_systematicsList, m_vbsLRJetHandle));
      
      // Intialise syst list (must come after all syst-aware inputs and outputs)
      ATH_CHECK (m_systematicsList.initialize());
      return StatusCode::SUCCESS;
    }

    StatusCode BaselineVarstwoLepAlg::execute(){
      // Loop over all systs
      for (const auto& sys : m_systematicsList.systematicsVector()){
        // Retrieve inputs
        const xAOD::EventInfo *event = nullptr;
        ANA_CHECK (m_eventHandle.retrieve (event, sys));

        const xAOD::JetContainer *smalljets = nullptr;
        ANA_CHECK (m_smalljetHandle.retrieve (smalljets, sys));
        
        const xAOD::JetContainer *largeJets = nullptr;
        ANA_CHECK (m_vbsLRJetHandle.retrieve (largeJets, sys));

        const xAOD::JetContainer *signalJets = nullptr;    
        if (m_doResolved){
          ANA_CHECK (m_signaljetHandle.retrieve (signalJets, sys));
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

        for (const std::string &string_var: m_floatVariables) {
          m_Fbranches.at(string_var).set(*event, -99., sys);
        }

        for (const auto& var: m_intVariables) {
          m_Ibranches.at(var).set(*event, -99, sys);
        }
        int n_largeJets = largeJets->size();
        int nCentralJets = 0;
        int nForwardJets = 0;

        bool WPgiven = !m_isBtag.empty();
        bool PCBTgiven = !m_PCBT.empty();
        
          int n_jets = smalljets->size();
          auto bjets = std::make_unique<ConstDataVector<xAOD::JetContainer>> (SG::VIEW_ELEMENTS);
        

          // number of central jets
          for(const xAOD::Jet* jet : *smalljets) {
            // count central jets
            if (std::abs(jet->eta())<2.5) nCentralJets++;
            else nForwardJets++;

          //  if (WPgiven) {
          //    if (m_isBtag.get(*jet, sys) && std::abs(jet->eta())<2.5) bjets->push_back(jet);
          //  }
          }
          
          for(const xAOD::Jet* jet : *signalJets) {
            if (WPgiven) {
              if (m_isBtag.get(*jet, sys)) bjets->push_back(jet);
            }
          }

          int n_bjets = bjets->size();

        m_Ibranches.at("nJets").set(*event, n_jets, sys);
        m_Ibranches.at("nBJets").set(*event, n_bjets, sys);  
        m_Ibranches.at("nCentralJets").set(*event, nCentralJets, sys);
        m_Ibranches.at("nForwardJets").set(*event, nForwardJets, sys);
        m_Ibranches.at("nLargeJets").set(*event, n_largeJets, sys);

        // selected leptons ;  
        const xAOD::Electron* ele0 = electrons->size() > 0 ? electrons->at(0) : nullptr;
        const xAOD::Electron* ele1 = electrons->size() > 1 ? electrons->at(1) : nullptr;
        
        const xAOD::Muon* mu0 = muons->size() > 0 ? muons->at(0) : nullptr;
        const xAOD::Muon* mu1 = muons->size() > 1 ? muons->at(1) : nullptr;
        /*
        if (!mu0 && !ele0) {
        ATH_MSG_WARNING("No valid lepton found!");
        return StatusCode::SUCCESS;
        }
        */
        if (muons->empty() && electrons->empty()) {
        ATH_MSG_WARNING("No valid lepton found!");
        return StatusCode::SUCCESS;
        }
        
        std::vector<std::pair<const xAOD::IParticle*, int>> leptons;
        if(ele0) leptons.emplace_back(ele0, -11*ele0->charge());
        if(mu0) leptons.emplace_back(mu0, -13*mu0->charge());
        if(ele1) leptons.emplace_back(ele1, -11*ele1->charge());
        if(mu1) leptons.emplace_back(mu1, -13*mu1->charge());

        int nLeptons = muons->size() + electrons->size();
        m_Ibranches.at("nLeptons").set(*event, nLeptons, sys);
        int n_electrons = electrons->size();
        int n_muons = muons->size();
        m_Ibranches.at("nElectrons").set(*event, n_electrons, sys);
        m_Ibranches.at("nMuons").set(*event, n_muons, sys);
        if (nLeptons <= 1) {
            // If the event somehow slipped through without 2 leptons, safely skip it
            ATH_MSG_WARNING("less than 2 leptons");
            return StatusCode::SUCCESS; 
        }

        std::sort(leptons.begin(), leptons.end(),
            [](const std::pair<const xAOD::IParticle*, int>& a,
               const std::pair<const xAOD::IParticle*, int>& b) {
              return a.first->pt() > b.first->pt(); });

        for(unsigned int i=0; i<std::min(size_t(2),leptons.size()); i++){
          std::string prefix = "Lepton"+std::to_string(i+1);
          TLorentzVector tlv = leptons[i].first->p4();
          int lep_pdgid = leptons[i].second;
          m_Fbranches.at(prefix+"_pt").set(*event, tlv.Pt(), sys);
          m_Fbranches.at(prefix+"_eta").set(*event, tlv.Eta(), sys);
          m_Fbranches.at(prefix+"_phi").set(*event, tlv.Phi(), sys);
          m_Fbranches.at(prefix+"_E").set(*event, tlv.E(), sys);
          m_Fbranches.at(prefix+"_m").set(*event, tlv.M(), sys);
          m_Fbranches.at(prefix+"_px").set(*event,  tlv.Px(), sys);
          m_Fbranches.at(prefix+"_py").set(*event,  tlv.Py(), sys);
          m_Fbranches.at(prefix+"_pz").set(*event,  tlv.Pz(), sys);
          m_Fbranches.at(prefix+"_y").set(*event,  tlv.Pz(), sys);

          float SF = 1.;
          if(std::abs(lep_pdgid)==11){
            int ele_ECIDS = m_eleECIDS.get(*leptons[i].first, sys);
            m_Ibranches.at(prefix+"_ele_ECIDS").set(*event, ele_ECIDS, sys);
          }
          if(m_isMC){
            if(std::abs(lep_pdgid)==11){
              SF = m_ele_SF.get(*leptons[i].first,sys);

              int ele_firstEgMotherTruthType = m_ele_firstEgMotherTruthType.get(*leptons[i].first,sys);
              m_Ibranches.at(prefix+"_ele_firstEgMotherTruthType").set(*event, ele_firstEgMotherTruthType, sys);

              int ele_firstEgMotherTruthOrigin = m_ele_firstEgMotherTruthOrigin.get(*leptons[i].first,sys);
              m_Ibranches.at(prefix+"_ele_firstEgMotherTruthOrigin").set(*event, ele_firstEgMotherTruthOrigin, sys);

              int ele_firstEgMotherPdgId = m_ele_firstEgMotherPdgId.get(*leptons[i].first,sys);
              m_Ibranches.at(prefix+"_ele_firstEgMotherPdgId").set(*event, ele_firstEgMotherPdgId, sys);   
            }
            else{
              SF = m_mu_SF.get(*leptons[i].first,sys);
            }
            m_Fbranches.at(prefix+"_effSF").set(*event, SF, sys);
          }
          int charge = lep_pdgid > 0 ? -1 : 1;
          m_Ibranches.at(prefix+"_charge").set(*event, charge, sys);
          m_Ibranches.at(prefix+"_pdgid").set(*event, lep_pdgid, sys);
        
          // leptons truth information
          if (m_isMC){
            int lep_truthOrigin = std::abs(lep_pdgid)==11 ?
              m_ele_truthOrigin.get(*leptons[i].first,sys) :
              m_mu_truthOrigin.get(*leptons[i].first,sys);
            m_Ibranches.at(prefix + "_truthOrigin").set(*event, lep_truthOrigin, sys);
            int lep_truthType = std::abs(lep_pdgid)==11 ?
              m_ele_truthType.get(*leptons[i].first,sys) :
              m_mu_truthType.get(*leptons[i].first,sys);
            m_Ibranches.at(prefix + "_truthType").set(*event, lep_truthType, sys);
            int lep_isPrompt = 0;
            if (std::abs(lep_pdgid)==13){ // simplistic
              if (lep_truthType==6) lep_isPrompt=1; // isolated prompts
            } else if (std::abs(lep_pdgid)==11){
              if (lep_truthType==2) lep_isPrompt=1; // isolated prompts
            }
            
            m_Ibranches.at(prefix + "_isPrompt").set(*event, lep_isPrompt, sys);
          }
          
        }//Leptons

        // dilepton kinematics
        TLorentzVector ll;
        TLorentzVector Leading_lep;
        TLorentzVector Subleading_lep;
        bool isSameFlavour = std::abs(leptons[0].second) == std::abs(leptons[1].second);
        
        if (isSameFlavour){
            Leading_lep = leptons[0].first->p4();
            Subleading_lep = leptons[1].first->p4();
            ll = Leading_lep + Subleading_lep;
        
        
        m_Fbranches.at("ll_m").set(*event, ll.M(), sys);
        m_Fbranches.at("ll_pt").set(*event, ll.Pt(), sys);
        m_Fbranches.at("ll_eta").set(*event, ll.Eta(), sys);
        m_Fbranches.at("ll_phi").set(*event, ll.Phi(), sys);
        m_Fbranches.at("ll_E").set(*event, ll.E(), sys);
        m_Fbranches.at("ll_px").set(*event, ll.Px(), sys);
        m_Fbranches.at("ll_py").set(*event, ll.Py(), sys);
        m_Fbranches.at("ll_pz").set(*event, ll.Pz(), sys);
        m_Fbranches.at("ll_y").set(*event, ll.Rapidity(), sys);    

        m_Fbranches.at("dRll").set(*event, Leading_lep.DeltaR(Subleading_lep), sys);
        m_Fbranches.at("dPhill").set(*event, Leading_lep.DeltaPhi(Subleading_lep), sys);
        m_Fbranches.at("dEtall").set(*event, abs(Leading_lep.Eta() - Subleading_lep.Eta()), sys);
        
        }

        if (m_doResolved){
          //jet sector
        for (std::size_t i=0; i<std::min(signalJets->size(),(std::size_t)2); i++){
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_pt").set(*event, signalJets->at(i)->pt(), sys);
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_eta").set(*event, signalJets->at(i)->eta(), sys);
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_phi").set(*event, signalJets->at(i)->phi(), sys);
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_E").set(*event, signalJets->at(i)->e(), sys);
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_m").set(*event, signalJets->at(i)->m(), sys);
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_px").set(*event, signalJets->at(i)->px(), sys);
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_py").set(*event, signalJets->at(i)->py(), sys);
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_pz").set(*event, signalJets->at(i)->pz(), sys);
          m_Fbranches.at("SignalJet"+std::to_string(i+1)+"_y").set(*event, signalJets->at(i)->rapidity(), sys);

            if(PCBTgiven)
              m_Ibranches.at("SignalJet"+std::to_string(i+1)+"_pcbt").set(*event, m_PCBT.get(*signalJets->at(i), sys),sys);

            if (WPgiven) {
                    // m_isBtag returns a char (1 for pass, 0 for fail), which casts safely to int
                    m_Ibranches.at("SignalJet" + std::to_string(i+1)+"_isBtagged").set(*event, m_isBtag.get(*signalJets->at(i), sys), sys);
                }
            if (m_isMC) {
              m_Ibranches.at("SignalJet"+std::to_string(i+1)+"_truthLabel").set(*event, m_truthFlav.get(*signalJets->at(i), sys), sys);
            }
          }

          TLorentzVector SDijet;
          TLorentzVector SJet1;
          TLorentzVector SJet2;
          if (signalJets->size() >=2){
            SDijet = signalJets->at(0)->p4()+ signalJets->at(1)->p4();
            SJet1 = signalJets->at(0)->p4();
            SJet2 = signalJets->at(1)->p4();
            m_Fbranches.at("Sdijet_m").set(*event, SDijet.M(), sys);
            m_Fbranches.at("Sdijet_pt").set(*event, SDijet.Pt(), sys);
            m_Fbranches.at("Sdijet_eta").set(*event, SDijet.Eta(), sys);
            m_Fbranches.at("Sdijet_phi").set(*event, SDijet.Phi(), sys);
            m_Fbranches.at("Sdijet_px").set(*event, SDijet.Px(), sys);
            m_Fbranches.at("Sdijet_py").set(*event, SDijet.Py(), sys);
            m_Fbranches.at("Sdijet_pz").set(*event, SDijet.Pz(), sys);
            m_Fbranches.at("Sdijet_E").set(*event, SDijet.E(), sys);
            m_Fbranches.at("Sdijet_y").set(*event, SDijet.Rapidity(), sys);
            m_Fbranches.at("dRSjj").set(*event, SJet1.DeltaR(SJet2), sys);
            m_Fbranches.at("dPhiSjj").set(*event, SJet1.DeltaPhi(SJet2), sys);
            m_Fbranches.at("dEtaSjj").set(*event, abs(SJet1.Eta() - SJet2.Eta()), sys);
        
          // leading signal-jet + leadinglepton sector
          if (isSameFlavour){

            TLorentzVector Sjjll = ll + SDijet;
            m_Fbranches.at("VjjVll_m").set(*event, Sjjll.M(), sys);
            m_Fbranches.at("VjjVll_pt").set(*event, Sjjll.Pt(), sys);
            m_Fbranches.at("VjjVll_px").set(*event, Sjjll.Px(), sys);
            m_Fbranches.at("VjjVll_py").set(*event, Sjjll.Py(), sys);
            m_Fbranches.at("VjjVll_pz").set(*event, Sjjll.Pz(), sys);
            m_Fbranches.at("VjjVll_E").set(*event, Sjjll.E(), sys);
            m_Fbranches.at("VjjVll_y").set(*event, Sjjll.Rapidity(), sys);
            m_Fbranches.at("VjjVll_eta").set(*event, Sjjll.Eta(), sys);
            m_Fbranches.at("VjjVll_phi").set(*event, Sjjll.Phi(), sys);
            m_Fbranches.at("dRVjjVll").set(*event, SDijet.DeltaR(ll), sys);
            m_Fbranches.at("dPhiVjjVll").set(*event, SDijet.DeltaPhi(ll), sys);
            m_Fbranches.at("dEtaVjjVll").set(*event, SDijet.Eta() - ll.Eta(), sys);
          }
        }
        

          //min Delta R (sjet, lepton)
          std::vector<double> deltaRs;
          for(unsigned int i=0; i<std::min(size_t(2),leptons.size()); i++){
            TLorentzVector tlv = leptons[i].first->p4();
            for (const auto& SJet : *signalJets) {
              deltaRs.push_back(SJet->p4().DeltaR(tlv));
            }
          }
          if (!deltaRs.empty()) {
            auto minDeltaR = *std::min_element(std::begin(deltaRs), std::end(deltaRs));
            m_Fbranches.at("dR_sjet_lepton_min").set(*event, minDeltaR, sys);
          }
        }
        // Large R jets kinematics
        if ( n_largeJets > 0){
          const xAOD::Jet* largeJet = largeJets->at(0);
          
          float wtagger_score = m_WTag_score.get(*largeJet, sys);
          float pqcdcx = m_GN3Xv01_pqcdcx.get(*largeJet, sys);
          float pWqq = m_GN3Xv01_pWqq.get(*largeJet, sys);
          float pqcdbx = m_GN3Xv01_pqcdbx.get(*largeJet, sys);
          float ptop = m_GN3Xv01_ptop.get(*largeJet, sys);

          m_Fbranches.at("LargeJet1_m").set(*event, largeJet->m(), sys);
          m_Fbranches.at("LargeJet1_pt").set(*event, largeJet->pt(), sys);
          m_Fbranches.at("LargeJet1_px").set(*event, largeJet->px(), sys);
          m_Fbranches.at("LargeJet1_py").set(*event, largeJet->py(), sys);
          m_Fbranches.at("LargeJet1_pz").set(*event, largeJet->pz(), sys);
          m_Fbranches.at("LargeJet1_eta").set(*event, largeJet->eta(), sys);
          m_Fbranches.at("LargeJet1_phi").set(*event, largeJet->phi(), sys);
          m_Fbranches.at("LargeJet1_E").set(*event, largeJet->e(), sys);
          m_Fbranches.at("LargeJet1_y").set(*event, largeJet->rapidity(), sys);
          
          m_Fbranches.at("LargeJet1_GN3X_pqcdcx").set(*event, pqcdcx, sys);
          m_Fbranches.at("LargeJet1_GN3X_pWqq").set(*event, pWqq, sys);
          m_Fbranches.at("LargeJet1_GN3X_pqcdbx").set(*event, pqcdbx, sys);
          m_Fbranches.at("LargeJet1_GN3X_ptop").set(*event, ptop, sys);
          
          m_Fbranches.at("LargeJet1_" + m_WTag_Type + m_WTag_WP + "Tagger_Score").set(*event, wtagger_score, sys);
          int pass_wtag = static_cast<int>(m_Pass_WTag.get(*largeJet, sys));
          m_Ibranches.at("LargeJet1_Pass_"+m_WTag_Type+m_WTag_WP).set(*event, pass_wtag, sys);
          

          if(isSameFlavour){
          
          TLorentzVector SJll = ll + largeJets->at(0)->p4();
            m_Fbranches.at("VJVll_m").set(*event, SJll.M(), sys);
            m_Fbranches.at("VJVll_pt").set(*event, SJll.Pt(), sys);
            m_Fbranches.at("VJVll_px").set(*event, SJll.Px(), sys);
            m_Fbranches.at("VJVll_py").set(*event, SJll.Py(), sys);
            m_Fbranches.at("VJVll_pz").set(*event, SJll.Pz(), sys);
            m_Fbranches.at("VJVll_E").set(*event, SJll.E(), sys);
            m_Fbranches.at("VJVll_y").set(*event, SJll.Rapidity(), sys);
            m_Fbranches.at("VJVll_eta").set(*event, SJll.Eta(), sys);
            m_Fbranches.at("VJVll_phi").set(*event, SJll.Phi(), sys);
            m_Fbranches.at("dRVJVll").set(*event, largeJets->at(0)->p4().DeltaR(ll), sys);
            m_Fbranches.at("dPhiVJVll").set(*event, largeJets->at(0)->p4().DeltaPhi(ll), sys);
            m_Fbranches.at("dEtaVJVll").set(*event, largeJets->at(0)->p4().Eta() - ll.Eta(), sys);
          }
          
        }

        // kinematics of vbs jets
        if(!m_UseVBFRNN){
          if ( vbsjets->size() >= 2 ){
            const xAOD::Jet* vbsJet1 = vbsjets->at(0);
            const xAOD::Jet* vbsJet2 = vbsjets->at(1);

          TLorentzVector vbs_jj = vbsJet1->p4() + vbsJet2->p4();
            
          m_Fbranches.at("VBSJ1_m").set(*event, vbsJet1->m(), sys);
          m_Fbranches.at("VBSJ1_pt").set(*event, vbsJet1->pt(), sys);
          m_Fbranches.at("VBSJ1_eta").set(*event, vbsJet1->eta(), sys);
          m_Fbranches.at("VBSJ1_phi").set(*event, vbsJet1->phi(), sys);
          m_Fbranches.at("VBSJ1_E").set(*event, vbsJet1->e(), sys);
          m_Fbranches.at("VBSJ1_px").set(*event, vbsJet1->px(), sys);
          m_Fbranches.at("VBSJ1_py").set(*event, vbsJet1->py(), sys);
          m_Fbranches.at("VBSJ1_pz").set(*event, vbsJet1->pz(), sys);
          m_Fbranches.at("VBSJ1_y").set(*event, vbsJet1->rapidity(), sys);

          m_Fbranches.at("VBSJ2_m").set(*event, vbsJet2->m(), sys);
          m_Fbranches.at("VBSJ2_pt").set(*event, vbsJet2->pt(), sys);
          m_Fbranches.at("VBSJ2_eta").set(*event, vbsJet2->eta(), sys);
          m_Fbranches.at("VBSJ2_phi").set(*event, vbsJet2->phi(), sys);
          m_Fbranches.at("VBSJ2_E").set(*event, vbsJet2->e(), sys);
          m_Fbranches.at("VBSJ2_px").set(*event, vbsJet2->px(), sys);
          m_Fbranches.at("VBSJ2_py").set(*event, vbsJet2->py(), sys);
          m_Fbranches.at("VBSJ2_pz").set(*event, vbsJet2->pz(), sys);
          m_Fbranches.at("VBSJ2_y").set(*event, vbsJet2->rapidity(), sys);

          m_Fbranches.at("VBSdijet_m").set(*event, vbs_jj.M(), sys);
          m_Fbranches.at("VBSdijet_pt").set(*event, vbs_jj.Pt(), sys);
          m_Fbranches.at("VBSdijet_eta").set(*event, vbs_jj.Eta(), sys);
          m_Fbranches.at("VBSdijet_phi").set(*event, vbs_jj.Phi(), sys);
          m_Fbranches.at("VBSdijet_E").set(*event, vbs_jj.E(), sys);
          m_Fbranches.at("VBSdijet_px").set(*event, vbs_jj.Px(), sys);
          m_Fbranches.at("VBSdijet_py").set(*event, vbs_jj.Py(), sys);
          m_Fbranches.at("VBSdijet_pz").set(*event, vbs_jj.Pz(), sys);
          m_Fbranches.at("VBSdijet_y").set(*event, vbs_jj.Rapidity(), sys);
          m_Fbranches.at("dRVBSjj").set(*event, vbsJet1->p4().DeltaR(vbsJet2->p4()), sys);
          m_Fbranches.at("dEtaVBSjj").set(*event, vbsJet1->eta()-vbsJet2->eta(), sys);
          m_Fbranches.at("dPhiVBSjj").set(*event, vbsJet1->p4().DeltaPhi(vbsJet2->p4()), sys);
            
          float vbs_eta1 = vbsJet1->eta();
          float vbs_eta2 = vbsJet2->eta();
          float vbs_eta_mean = (vbs_eta1 + vbs_eta2) / 2.0;
          float vbs_delta_eta = std::abs(vbs_eta1 - vbs_eta2);

          // Protect against division by zero
          if (vbs_delta_eta > 0.0) {
                
          // 1. Centrality of the Z boson (ll system)
          if (isSameFlavour) { // isSF ensures 'll' was properly built
                    float cent_Vll = (ll.Eta() - vbs_eta_mean) / vbs_delta_eta;
                    m_Fbranches.at("ZeppCent_Vll").set(*event, cent_Vll, sys);
          }

          // 2. Centrality of the Hadronic V (Resolved Dijet)
          if (m_doResolved && signalJets->size() >= 2) {
            float cent_Vjj_res = ((signalJets->at(0)->p4()+ signalJets->at(1)->p4()).Eta() - vbs_eta_mean) / vbs_delta_eta;
            m_Fbranches.at("ZeppCent_Vjj_res").set(*event, cent_Vjj_res, sys);
            }

          // 3. Centrality of the Hadronic V (Merged Fat Jet)
            if (n_largeJets > 0) {
                float cent_VJ_merg = (largeJets->at(0)->eta() - vbs_eta_mean) / vbs_delta_eta;
                m_Fbranches.at("ZeppCent_VJ_merg").set(*event, cent_VJ_merg, sys);
              }
            }
              
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
              m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_px").set(*event, RNNJet->px(), sys);
              m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_py").set(*event, RNNJet->py(), sys);
              m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_pz").set(*event, RNNJet->pz(), sys);
              m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_E").set(*event, RNNJet->e(), sys);
              m_Fbranches.at(RNNJets_container_name+"_"+prefix+"_y").set(*event, RNNJet->rapidity(), sys);
              
              if (RNNJets_container->size() >= 2) {
                TLorentzVector rnn_jj = RNNJets_container->at(0)->p4() + RNNJets_container->at(1)->p4();
                m_Fbranches.at(RNNJets_container_name+"_dijet_m").set(*event, rnn_jj.M(), sys);
                m_Fbranches.at(RNNJets_container_name+"_dijet_pt").set(*event, rnn_jj.Pt(), sys);
                m_Fbranches.at(RNNJets_container_name+"_dijet_phi").set(*event, rnn_jj.Phi(), sys);
                m_Fbranches.at(RNNJets_container_name+"_dijet_eta").set(*event, rnn_jj.Eta(), sys);
                m_Fbranches.at(RNNJets_container_name+"_dijet_px").set(*event, rnn_jj.Px(), sys);
                m_Fbranches.at(RNNJets_container_name+"_dijet_py").set(*event, rnn_jj.Py(), sys);
                m_Fbranches.at(RNNJets_container_name+"_dijet_pz").set(*event, rnn_jj.Pz(), sys);
                m_Fbranches.at(RNNJets_container_name+"_dijet_E").set(*event, rnn_jj.E(), sys);
                m_Fbranches.at(RNNJets_container_name+"_dijet_y").set(*event, rnn_jj.Rapidity(), sys);
                m_Fbranches.at("dEta"+RNNJets_container_name).set(*event, std::abs(RNNJets_container->at(0)->eta() - RNNJets_container->at(1)->eta()), sys);
                m_Fbranches.at("dPhi"+RNNJets_container_name).set(*event, (RNNJets_container->at(0)->p4()).DeltaPhi(RNNJets_container->at(1)->p4()), sys);
                m_Fbranches.at("dR"+RNNJets_container_name).set(*event, (RNNJets_container->at(0)->p4()).DeltaR(RNNJets_container->at(1)->p4()), sys);   
                }
              
            }
          }
        }
       
       if (m_UseVBFRNN){
            
            // A. Boosted (Merged) Topology
            if (RNNJets_boosted && RNNJets_boosted->size() >= 2) {
                float vbs_eta_mean = (RNNJets_boosted->at(0)->eta() + RNNJets_boosted->at(1)->eta()) / 2.0;
                float vbs_delta_eta = std::abs(RNNJets_boosted->at(0)->eta() - RNNJets_boosted->at(1)->eta());

                if (vbs_delta_eta > 0.0) {
                    // Centrality of the Z boson relative to the boosted RNN jets
                    if (isSameFlavour){ 
                      float cent_Vll_RNN_boosted = (ll.Eta() - vbs_eta_mean) / vbs_delta_eta;
                      m_Fbranches.at("ZeppCent_Vll_RNN_boosted").set(*event, cent_Vll_RNN_boosted, sys);
                        }
                    // Centrality of the Fat Jet relative to the boosted RNN jets
                    if (n_largeJets > 0) {
                        float cent_VJ_merg_RNN = (largeJets->at(0)->eta() - vbs_eta_mean) / vbs_delta_eta;
                        m_Fbranches.at("ZeppCent_VJ_merg_RNN").set(*event, cent_VJ_merg_RNN, sys);
                        }
                }
            }

            // B. Resolved Topology
            if (m_doResolved && RNNJets_resolved && RNNJets_resolved->size() >= 2) {
                float vbs_eta_mean = (RNNJets_resolved->at(0)->eta() + RNNJets_resolved->at(1)->eta()) / 2.0;
                float vbs_delta_eta = std::abs(RNNJets_resolved->at(0)->eta() - RNNJets_resolved->at(1)->eta());

                if (vbs_delta_eta > 0.0) {
                    // Centrality of the Z boson relative to the resolved RNN jets
                    if (isSameFlavour){
                        float cent_Vll_RNN_resolved = (ll.Eta() - vbs_eta_mean) / vbs_delta_eta;
                        m_Fbranches.at("ZeppCent_Vll_RNN_resolved").set(*event, cent_Vll_RNN_resolved, sys);
                        }
                    // Centrality of the Hadronic Dijet relative to the resolved RNN jets
                    if (signalJets->size() >= 2){
                        float cent_Vjj_res_RNN = ((signalJets->at(0)->p4()+ signalJets->at(1)->p4()).Eta() - vbs_eta_mean) / vbs_delta_eta;
                        m_Fbranches.at("ZeppCent_Vjj_res_RNN").set(*event, cent_Vjj_res_RNN, sys);
                        }
                }
            }
       
        }
        
      }

      return StatusCode::SUCCESS;
    }

}
