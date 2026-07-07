/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


#include "BaselineVarsoneLepAlg.h"
#include "AthContainers/AuxElement.h"
#include "TLorentzVector.h"
#include <AthContainers/ConstDataVector.h>
#include <AthenaKernel/Units.h>

namespace VBSSEMILEP{
    StatusCode BaselineVarsoneLepAlg::initialize(){
      ATH_MSG_INFO("*********************************\n");
      ATH_MSG_INFO("       oneLepBaselineVarsAlg    \n");
      ATH_MSG_INFO("*********************************\n");

      // Read syst-aware input handles
      ATH_CHECK (m_vbsLRJetHandle.initialize(m_systematicsList));
      ATH_CHECK (m_signaljetHandle.initialize(m_systematicsList));
      ATH_CHECK (m_vbsJetHandle.initialize(m_systematicsList));
      ATH_CHECK (m_vbsElectronHandle.initialize(m_systematicsList));
      ATH_CHECK (m_vbsMuonHandle.initialize(m_systematicsList));
      ATH_CHECK (m_metHandle.initialize(m_systematicsList));
      ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

      if(m_isMC){
        ATH_CHECK (m_ele_truthOrigin.initialize(m_systematicsList, m_vbsElectronHandle));
        ATH_CHECK (m_ele_truthType.initialize(m_systematicsList, m_vbsElectronHandle));
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
        ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_signaljetHandle));
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

      ATH_CHECK (m_eleECIDS.initialize(m_systematicsList, m_vbsElectronHandle));
      
      ATH_CHECK (m_METSig.initialize(m_systematicsList, m_metHandle));

      ATH_CHECK (m_GN2Xv01_phbb.initialize(m_systematicsList, m_vbsLRJetHandle));
      ATH_CHECK (m_GN2Xv01_phcc.initialize(m_systematicsList, m_vbsLRJetHandle));
      ATH_CHECK (m_GN2Xv01_pqcd.initialize(m_systematicsList, m_vbsLRJetHandle));
      ATH_CHECK (m_GN2Xv01_ptop.initialize(m_systematicsList, m_vbsLRJetHandle));

      // Intialise syst list (must come after all syst-aware inputs and outputs)
      ATH_CHECK (m_systematicsList.initialize());
      return StatusCode::SUCCESS;
    }

    StatusCode BaselineVarsoneLepAlg::execute(const EventContext& ctx){
      // Loop over all systs
      for (const auto& sys : m_systematicsList.systematicsVector()){
        // Retrieve inputs
        const xAOD::EventInfo *event = nullptr;
        ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

        const xAOD::JetContainer *largeJets = nullptr;
        ANA_CHECK (m_vbsLRJetHandle.retrieve (largeJets, sys, ctx));

        const xAOD::JetContainer *signalJets = nullptr;
        ANA_CHECK (m_signaljetHandle.retrieve (signalJets, sys, ctx));

        const xAOD::JetContainer *vbsjets = nullptr;
        ANA_CHECK (m_vbsJetHandle.retrieve (vbsjets, sys, ctx));

        const xAOD::MuonContainer *muons = nullptr;
        ANA_CHECK (m_vbsMuonHandle.retrieve (muons, sys, ctx));

        const xAOD::ElectronContainer *electrons = nullptr;
        ANA_CHECK (m_vbsElectronHandle.retrieve (electrons, sys, ctx));

        const xAOD::MissingETContainer *metCont = nullptr;
        ANA_CHECK (m_metHandle.retrieve (metCont, sys, ctx));
        const xAOD::MissingET* met = (*metCont)["Final"];
        if (!met) {
          ATH_MSG_ERROR("Could not retrieve MET");
          return StatusCode::FAILURE;	
        }
        for (const std::string &string_var: m_floatVariables) {
          m_Fbranches.at(string_var).set(*event, -99., sys);
        }

        for (const auto& var: m_intVariables) {
          m_Ibranches.at(var).set(*event, -99, sys);
        }
        
        int n_jets = signalJets->size() + vbsjets->size();
        int n_largeJets = largeJets->size();
        int nCentralJets = 0;
        int nForwardJets = 0;

        int n_electrons = electrons->size();
        int n_muons = muons->size();

        // b-jet sector
        bool WPgiven = !m_isBtag.empty();
        bool PCBTgiven = !m_PCBT.empty();
        auto bjets = std::make_unique<ConstDataVector<xAOD::JetContainer>> (SG::VIEW_ELEMENTS);

        // number of central jets
        for(const xAOD::Jet* jet : *signalJets) {
          // count central jets
          if (std::abs(jet->eta())<2.5) nCentralJets++;
          else nForwardJets++;

          if (WPgiven) {
            if (m_isBtag.get(*jet, sys) && std::abs(jet->eta())<2.5) bjets->push_back(jet);
          }
        }

        for(const xAOD::Jet* vbsjet : *vbsjets) {
          if (std::abs(vbsjet->eta())<2.5) nCentralJets++;
          else nForwardJets++;
        }

        int n_bjets = bjets->size();

        m_Ibranches.at("nJets").set(*event, n_jets, sys);
        m_Ibranches.at("nLargeJets").set(*event, n_largeJets, sys);
        m_Ibranches.at("nElectrons").set(*event, n_electrons, sys);
        m_Ibranches.at("nMuons").set(*event, n_muons, sys);
        m_Ibranches.at("nBJets").set(*event, n_bjets, sys);
        m_Ibranches.at("nCentralJets").set(*event, nCentralJets, sys);
        m_Ibranches.at("nForwardJets").set(*event, nForwardJets, sys);
        m_Ibranches.at("nLeptons").set(*event, n_muons + n_electrons, sys);

        // selected leptons ;
        const xAOD::Electron* ele0 = n_electrons>=1 ? electrons->at(0) : nullptr;
        if (!ele0) {
        ATH_MSG_WARNING("No valid electron found!");
        return StatusCode::SUCCESS;
        }

        const xAOD::Muon* mu0 = n_muons>=1 ? muons->at(0) : nullptr;
        if (!mu0) {
        ATH_MSG_WARNING("No valid muon found!");
        return StatusCode::SUCCESS;
        }

        std::vector<std::pair<const xAOD::IParticle*, int>> leptons;
        if(ele0) leptons.emplace_back(ele0, -11*ele0->charge());
        if(mu0) leptons.emplace_back(mu0, -13*mu0->charge());

        std::sort(leptons.begin(), leptons.end(),
            [](const std::pair<const xAOD::IParticle*, int>& a,
               const std::pair<const xAOD::IParticle*, int>& b) {
              return a.first->pt() > b.first->pt(); });

        TLorentzVector metVec;
        metVec.SetPtEtaPhiE(met->met(), 0, met->phi(), met->met());
        
        if( !leptons.empty() ){
          TLorentzVector tlv = leptons[0].first->p4();
          int lep_pdgid = leptons[0].second;
          m_Fbranches.at("Lepton_pt").set(*event,  tlv.Pt(), sys);
          m_Fbranches.at("Lepton_eta").set(*event, tlv.Eta(), sys);
          m_Fbranches.at("Lepton_phi").set(*event, tlv.Phi(), sys);
          m_Fbranches.at("Lepton_E").set(*event, tlv.E(), sys);
          m_Fbranches.at("Lepton_m").set(*event, tlv.M(), sys);
          m_Fbranches.at("Lepton_px").set(*event,  tlv.Px(), sys);
          m_Fbranches.at("Lepton_py").set(*event,  tlv.Py(), sys);
          m_Fbranches.at("Lepton_pz").set(*event,  tlv.Pz(), sys);
          m_Fbranches.at("Lepton_y").set(*event,  tlv.Rapidity(), sys);

          m_Fbranches.at("dPhilMET").set(*event, tlv.DeltaPhi(metVec), sys);
	        
          float mt_lept_met = std::sqrt(2 * met->met() * tlv.Pt() * (1 - std::cos(tlv.DeltaPhi(metVec))));
          m_Fbranches.at("Lepton_MET_mT").set(*event, mt_lept_met, sys);

          if(m_isMC){
            float SF = -99;
            if(std::abs(leptons[0].second)==11 ){
              SF = m_ele_SF.get(*leptons[0].first,sys);

              int ele_ECIDS = m_eleECIDS.get(*leptons[0].first, sys);
              m_Ibranches.at("Lepton_ele_ECIDS").set(*event, ele_ECIDS, sys);
            }
            else{
              SF = m_mu_SF.get(*leptons[0].first,sys);
            }
            m_Fbranches.at("Lepton_effSF").set(*event, SF, sys);
          }
          int charge = leptons[0].second>0 ? -1 : 1;
          m_Ibranches.at("Lepton_charge").set(*event, charge, sys);
          m_Ibranches.at("Lepton_pdgid").set(*event, lep_pdgid, sys);
        
          // leptons truth information
          if (m_isMC){
            int lep_truthOrigin = std::abs(lep_pdgid)==11 ?
              m_ele_truthOrigin.get(*leptons[0].first,sys) :
              m_mu_truthOrigin.get(*leptons[0].first,sys);
            m_Ibranches.at("Lepton_truthOrigin").set(*event, lep_truthOrigin, sys);
            int lep_truthType = std::abs(lep_pdgid)==11 ?
              m_ele_truthType.get(*leptons[0].first,sys) :
              m_mu_truthType.get(*leptons[0].first,sys);
            m_Ibranches.at("Lepton_truthType").set(*event, lep_truthType, sys);
            int lep_isPrompt = 0;
            if (std::abs(lep_pdgid)==13){ // simplistic
              if (lep_truthType==6) lep_isPrompt=1; // isolated prompts
            } else if (std::abs(lep_pdgid)==11){
              if (lep_truthType==2) lep_isPrompt=1; // isolated prompts
            }
            
            m_Ibranches.at("Lepton_isPrompt").set(*event, lep_isPrompt, sys);
          }
          
        }

        //MET Significance 
        float METSig = m_METSig.get(*met, sys);
        m_Fbranches.at("METSig").set(*event, METSig, sys);
        
        // Reconstruct W boson (lepton + MET)
        TLorentzVector pW = (ele0) ? reconstructWBoson(ele0, met) : reconstructWBosonMu(mu0, met);
        m_Fbranches.at("Wlep_m").set(*event, pW.M(), sys);
        m_Fbranches.at("Wlep_pt").set(*event, pW.Pt(), sys);
        m_Fbranches.at("Wlep_px").set(*event, pW.Px(), sys);
        m_Fbranches.at("Wlep_py").set(*event, pW.Py(), sys);
        m_Fbranches.at("Wlep_pz").set(*event, pW.Pz(), sys);
        m_Fbranches.at("Wlep_E").set(*event, pW.E(), sys);
        m_Fbranches.at("Wlep_eta").set(*event, pW.Eta(), sys);
        m_Fbranches.at("Wlep_phi").set(*event, pW.Phi(), sys);
        m_Fbranches.at("Wlep_y").set(*event, pW.Y(), sys);
        int Wcharge = (ele0) ? ele0->charge() : mu0->charge();
        m_Ibranches.at("Wlep_charge").set(*event,  Wcharge, sys);

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

          if (m_isMC) {
            m_Ibranches.at("SignalJet"+std::to_string(i+1)+"_truthLabel").set(*event, m_truthFlav.get(*signalJets->at(i), sys), sys);
          }
          
        }

        if (signalJets->size() >=2){
          TLorentzVector SDijet = signalJets->at(0)->p4()+ signalJets->at(1)->p4();
          TLorentzVector SJet1 = signalJets->at(0)->p4();
          TLorentzVector SJet2 = signalJets->at(1)->p4();
          m_Fbranches.at("Sdijet_m").set(*event, SDijet.M(), sys);
          m_Fbranches.at("Sdijet_pt").set(*event, SDijet.Pt(), sys);
          m_Fbranches.at("Sdijet_px").set(*event, SDijet.Px(), sys);
          m_Fbranches.at("Sdijet_py").set(*event, SDijet.Py(), sys);
          m_Fbranches.at("Sdijet_pz").set(*event, SDijet.Pz(), sys);
          m_Fbranches.at("Sdijet_E").set(*event, SDijet.E(), sys);
          m_Fbranches.at("Sdijet_eta").set(*event, SDijet.Eta(), sys);
          m_Fbranches.at("Sdijet_phi").set(*event, SDijet.Phi(), sys);
          m_Fbranches.at("Sdijet_y").set(*event, SDijet.Y(), sys);
          m_Fbranches.at("dRSjj").set(*event, SJet1.DeltaR(SJet2), sys);
          m_Fbranches.at("dPhiSjj").set(*event, SJet1.DeltaPhi(SJet2), sys);
          m_Fbranches.at("dEtaSjj").set(*event, SJet1.Eta() - SJet2.Eta(), sys);
        } else {
            ATH_MSG_WARNING("Fewer than 2 signal jets; skipping Z reconstruction");
            continue;
        }

        //min Delta R (bjet, lepton)
        std::vector<double> deltaRs;
        if( !leptons.empty() ){
          TLorentzVector tlv = leptons[0].first->p4();
          for (const auto& SJet : *signalJets) {
            deltaRs.push_back(SJet->p4().DeltaR(tlv));
          }
        }
        if (!deltaRs.empty()) {
          auto minDeltaR = *std::min_element(std::begin(deltaRs), std::end(deltaRs));
          m_Fbranches.at("dRbl_min").set(*event, minDeltaR, sys);
        }

        // Large R jets kinematics
        if ( n_largeJets >= 1 ){
          const xAOD::Jet* largeJet = largeJets->at(0);
          float phbb = m_GN2Xv01_phbb.get(*largeJet, sys);
          float phcc = m_GN2Xv01_phcc.get(*largeJet, sys);
          float pqcd = m_GN2Xv01_pqcd.get(*largeJet, sys);
          float ptop = m_GN2Xv01_ptop.get(*largeJet, sys);
          float fbb = 0.25;
          float ftop = 0.25;
          float fqcd = 0.50;
          float pBkg = ptop + (1 - (phbb+phcc+pqcd));
          float ZScore = log(phcc / (fbb*phbb + ftop*ptop + fqcd*pqcd));
          
          m_Fbranches.at("LargeJet1_m").set(*event, largeJet->m(), sys);
          m_Fbranches.at("LargeJet1_pt").set(*event, largeJet->pt(), sys);
          m_Fbranches.at("LargeJet1_px").set(*event, largeJet->px(), sys);
          m_Fbranches.at("LargeJet1_py").set(*event, largeJet->py(), sys);
          m_Fbranches.at("LargeJet1_pz").set(*event, largeJet->pz(), sys);
          m_Fbranches.at("LargeJet1_eta").set(*event, largeJet->eta(), sys);
          m_Fbranches.at("LargeJet1_phi").set(*event, largeJet->phi(), sys);
          m_Fbranches.at("LargeJet1_E").set(*event, largeJet->e(), sys);
          m_Fbranches.at("LargeJet1_y").set(*event, largeJet->rapidity(), sys);

          m_Fbranches.at("LargeJet1_phbb").set(*event, phbb, sys);
          m_Fbranches.at("LargeJet1_phcc").set(*event, phcc, sys);
          m_Fbranches.at("LargeJet1_pqcd").set(*event, pqcd, sys);
          m_Fbranches.at("LargeJet1_ptop").set(*event, ptop, sys);
          m_Fbranches.at("LargeJet1_pBkg").set(*event, pBkg, sys);
          m_Fbranches.at("LargeJet1_ZScore").set(*event, ZScore, sys);
        } else {
            ATH_MSG_WARNING("No fat jet");
            continue;
        }

        // kinematics of vbs jets
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
          m_Fbranches.at("VBSdijet_y").set(*event, vbs_jj.Y(), sys);
          m_Fbranches.at("dRVBSjj").set(*event, vbsJet1->p4().DeltaR(vbsJet2->p4()), sys);
          m_Fbranches.at("dEtaVBSjj").set(*event, vbsJet1->eta()-vbsJet2->eta(), sys);
          m_Fbranches.at("dPhiVBSjj").set(*event, vbsJet1->p4().DeltaPhi(vbsJet2->p4()), sys);
          
        } else {
            ATH_MSG_WARNING("Fewer than 2 tagging jets");
            continue;
        }
      }
      
      return StatusCode::SUCCESS;
    }
    
    TLorentzVector BaselineVarsoneLepAlg::reconstructWBoson(const xAOD::Electron* electron, const xAOD::MissingET* met) {
    TLorentzVector pLep = electron->p4();
    TLorentzVector pNu;
    pNu.SetPtEtaPhiM(met->met(), 0.0, met->phi(), 0.0);
    return pLep + pNu;
    }
    
TLorentzVector BaselineVarsoneLepAlg::reconstructWBosonMu(const xAOD::Muon* muon, const xAOD::MissingET* met) {
    ATH_MSG_DEBUG("Reconstructing W boson with muon and MET");
    if (!muon) {
        ATH_MSG_ERROR("Muon pointer is null!");
        return TLorentzVector();
    }
    if (!met) {
        ATH_MSG_ERROR("MET pointer is null!");
        return TLorentzVector();
    }
    TLorentzVector pLep = muon->p4();
    TLorentzVector pNu;
    pNu.SetPtEtaPhiM(met->met(), 0.0, met->phi(), 0.0);
    return pLep + pNu;
}
    
}
