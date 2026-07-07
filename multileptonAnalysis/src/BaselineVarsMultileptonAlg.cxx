/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "AthContainers/AuxElement.h"
#include "BaselineVarsMultileptonAlg.h"

#include "EasyjetHub/MT2_ROOT.h"
#include "Math/VectorUtil.h"
#include "SubChannelClassify.h"
#include "TLorentzVector.h"

namespace MULTILEPTON
{
  StatusCode BaselineVarsMultileptonAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("   BaselineVarsMultileptonAlg    \n");
    ATH_MSG_INFO("*********************************\n");

    // Read syst-aware input handles
    ATH_CHECK(m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_tauHandle.initialize(m_systematicsList));
    ATH_CHECK(m_metHandle.initialize(m_systematicsList));
    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_kfJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_KF_MBB.initialize(m_systematicsList, m_eventHandle));

    // Initialise syst-aware output decorators
    for(const std::string &var : m_floatVariables){
      CP::SysWriteDecorHandle<float> whandle{var+"_%SYS%", this};
      m_Fbranches.emplace(var, whandle);
      ATH_CHECK(m_Fbranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }
    
    for(const std::string &var : m_intVariables){
      CP::SysWriteDecorHandle<int> whandle{var+"_%SYS%", this};
      m_Ibranches.emplace(var, whandle);
      ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    if (!m_isBtag.empty()){
      ATH_CHECK(m_isBtag.initialize(m_systematicsList, m_jetHandle));
    }

    ATH_CHECK (m_nmuons.initialize(m_systematicsList, m_jetHandle));

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());


    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsMultileptonAlg::execute(const EventContext& ctx)
  {
    // Loop over all systematics
    for (const auto& sys : m_systematicsList.systematicsVector()){
      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys, ctx));

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK (m_muonHandle.retrieve (muons, sys, ctx));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK (m_electronHandle.retrieve (electrons, sys, ctx));

      const xAOD::TauJetContainer *taus = nullptr;
      ANA_CHECK (m_tauHandle.retrieve (taus, sys, ctx));

      // Retrieve KF jets container (separate from original)
      const xAOD::JetContainer *KFJets = nullptr;
      if (m_doKinematicFit)
      {
        ANA_CHECK(m_kfJetHandle.retrieve(KFJets, sys, ctx));
      }

      for (const std::string &string_var: m_floatVariables) {
        m_Fbranches.at(string_var).set(*event, -999., sys);
      }

      for (const auto& var: m_intVariables) {
        m_Ibranches.at(var).set(*event, -999, sys);
      }

      int n_electrons = electrons->size();
      int n_muons = muons->size();
      int n_taus = taus->size();
      int n_totalJets = jets->size();
      int n_lightJets = 0;
      int n_centralJets = 0;
      int n_bjets = 0;

      // seperate jets into b-jets and light jets
      bool WPgiven = !m_isBtag.empty();
      const xAOD::Jet *centralJet1 = nullptr;
      const xAOD::Jet *centralJet2 = nullptr;
      const xAOD::Jet *bjet1 = nullptr;
      const xAOD::Jet *bjet2 = nullptr;
      
      for(const xAOD::Jet* jet : *jets) {
        // count central jets
        if (std::abs(jet->eta())<2.5) {
          n_centralJets++;
          if (!centralJet1)
          {
            centralJet1 = jet;
          }
          else if (!centralJet2)
          {
            centralJet2 = jet;
          }

          if (WPgiven && m_isBtag.get(*jet, sys)){
            ++n_bjets;
            if (!bjet1)
            {
              bjet1 = jet;
            }
            else if (!bjet2)
            {
              bjet2 = jet;
            }
          }
          else {
            n_lightJets++;
          }
        }
        else {
          n_lightJets++;
        }
      }

      // Select Higgs candidate jets from ORIGINAL container
      const xAOD::Jet *Hbb_Jet1 = nullptr;
      const xAOD::Jet *Hbb_Jet2 = nullptr;
      if (n_centralJets >= 2)
      {
        if (n_bjets == 0)
        {
          Hbb_Jet1 = centralJet1;
          Hbb_Jet2 = centralJet2;
        }
        else if (n_bjets == 1)
        {
          Hbb_Jet1 = bjet1;
          Hbb_Jet2 = (centralJet1 == Hbb_Jet1) ? centralJet2 : centralJet1;
        }
        else
        {
          Hbb_Jet1 = bjet1;
          Hbb_Jet2 = bjet2;
        }
      }

      // === KF JETS SELECTION (apply IDENTICAL logic to KF container) ===
      const xAOD::Jet *Hbb_KFJet1 = nullptr;
      const xAOD::Jet *Hbb_KFJet2 = nullptr;
      
      if (m_doKinematicFit && KFJets && KFJets->size() >= 2)
      {
        const xAOD::Jet *KF_centralJet1 = nullptr;
        const xAOD::Jet *KF_centralJet2 = nullptr;
        const xAOD::Jet *KF_bjet1 = nullptr;
        const xAOD::Jet *KF_bjet2 = nullptr;
        int n_KF_centralJets = 0;
        int n_KF_bjets = 0;

        // Apply SAME selection criteria to KF jets
        for (const xAOD::Jet *KFjet : *KFJets)
        {
          if (std::abs(KFjet->eta()) < 2.5)
          {
            n_KF_centralJets++;

            if (!KF_centralJet1)
            {
              KF_centralJet1 = KFjet;
            }
            else if (!KF_centralJet2)
            {
              KF_centralJet2 = KFjet;
            }

            if (WPgiven && m_isBtag.get(*KFjet, sys))
            {
              ++n_KF_bjets;
              if (!KF_bjet1)
              {
                KF_bjet1 = KFjet;
              }
              else if (!KF_bjet2)
              {
                KF_bjet2 = KFjet;
              }
            }
          }
        }

        if (n_KF_centralJets >= 2)
        {
          if (n_KF_bjets == 0)
          {
            Hbb_KFJet1 = KF_centralJet1;
            Hbb_KFJet2 = KF_centralJet2;
          }
          else if (n_KF_bjets == 1)
          {
            Hbb_KFJet1 = KF_bjet1;
            Hbb_KFJet2 = (KF_centralJet1 == Hbb_KFJet1) ? KF_centralJet2 : KF_centralJet1;
          }
          else
          {
            Hbb_KFJet1 = KF_bjet1;
            Hbb_KFJet2 = KF_bjet2;
          }
        }
      }

      m_Ibranches.at("nTotalJets").set(*event, n_totalJets, sys);
      m_Ibranches.at("nElectrons").set(*event, n_electrons, sys);
      m_Ibranches.at("nMuons").set(*event, n_muons, sys);
      m_Ibranches.at("nTaus").set(*event, n_taus, sys);
      m_Ibranches.at("nLightJets").set(*event, n_lightJets, sys);
      m_Ibranches.at("nBJets").set(*event, n_bjets, sys);
      m_Ibranches.at("nCentralJets").set(*event, n_centralJets, sys);

      auto classifier = SubChannelClassify(
          muons, electrons, taus, n_bjets
          );

      m_Ibranches.at("nLeptons").set(
          *event, classifier.getNLeptons(), sys);
      m_Ibranches.at("totalLepCharge").set(
          *event, classifier.getTotalChargeLep(), sys);
      m_Ibranches.at("totalTauCharge").set(
          *event, classifier.getTotalChargeTau(), sys);
      m_Ibranches.at("subChannelID").set(
          *event, static_cast<int>(classifier.getSubChannelId()), sys);
      m_Ibranches.at("subChannelFlavor").set(
          *event, classifier.getSubChannelFlavor(), sys);

      if (m_save_extrabb4l_vars)
      {
        if (!Hbb_Jet1 || !Hbb_Jet2)
        {
          ATH_MSG_DEBUG("One or both Hbb jets are null pointers. Hbb_Jet1: "
                          << (Hbb_Jet1 ? "valid" : "null")
                          << ", Hbb_Jet2: " << (Hbb_Jet2 ? "valid" : "null"));
          continue;
        }

        std::array<const xAOD::Jet*, 2> originalHbbJets = {Hbb_Jet1, Hbb_Jet2};

        if (m_doKinematicFit && Hbb_KFJet1 && Hbb_KFJet2) {
          TLorentzVector test_h_bb_kf = Hbb_KFJet1->p4() + Hbb_KFJet2->p4();
          if (std::isnan(test_h_bb_kf.M()) || std::isnan(test_h_bb_kf.Pt()) || 
              std::isnan(test_h_bb_kf.Eta()) || std::isnan(test_h_bb_kf.Phi())) {
              continue;
          } else {
              std::array<const xAOD::Jet*, 2> KFHbbJets = {Hbb_KFJet1, Hbb_KFJet2};
              ANA_CHECK(SaveExtrabb4lVars(originalHbbJets, KFHbbJets, event, sys));
          }
        }
      }
    }
    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsMultileptonAlg::SaveExtrabb4lVars(
    const std::array<const xAOD::Jet*, 2> &Hbbjets,
    const std::array<const xAOD::Jet*, 2> &KFJets, 
    const xAOD::EventInfo *event, 
    const auto& sys)
  {
    // Cache all p4 variants once for both jets to avoid redundant calls
    std::array<TLorentzVector, 2> jet_tlvs;
    std::array<xAOD::JetFourMom_t, 2> jet_uncorr;
    std::array<xAOD::JetFourMom_t, 2> jet_muonCorr;

    for(unsigned int i = 0; i < 2; i++){
        const xAOD::Jet* jet = Hbbjets[i];
        
        // Cache all three p4 variants once
        jet_tlvs[i] = jet->p4();
        jet_uncorr[i] = jet->jetP4("NoBJetCalibMomentum");
        jet_muonCorr[i] = jet->jetP4("MuonCorrMomentum");
        
        std::string prefix_bjet = "HbbCandidate_Jet" + std::to_string(i+1);
        
        m_Ibranches.at(prefix_bjet + "_n_muons").set(*event, m_nmuons.get(*jet, sys), sys);
        
        m_Fbranches.at(prefix_bjet + "_uncorrPt").set(*event, jet_uncorr[i].Pt(), sys);
        m_Fbranches.at(prefix_bjet + "_muonCorrPt").set(*event, jet_muonCorr[i].Pt(), sys);
        
        m_Fbranches.at(prefix_bjet + "_pt").set(*event, jet_tlvs[i].Pt(), sys);
        m_Fbranches.at(prefix_bjet + "_eta").set(*event, jet_tlvs[i].Eta(), sys);
        m_Fbranches.at(prefix_bjet + "_phi").set(*event, jet_tlvs[i].Phi(), sys);
        m_Fbranches.at(prefix_bjet + "_E").set(*event, jet_tlvs[i].E(), sys);
    }

    // Use cached values for member variables
    m_jet1_uncorr = jet_uncorr[0];
    m_jet2_uncorr = jet_uncorr[1];
    m_jet1_muonCorr = jet_muonCorr[0];
    m_jet2_muonCorr = jet_muonCorr[1];

    // NoBjetCalibCorrection
    xAOD::JetFourMom_t H_bb_uncorr = m_jet1_uncorr + m_jet2_uncorr;
    std::string prefix = "HbbCand_uncorr_";
    m_Fbranches.at(prefix + "m_bb").set(*event, H_bb_uncorr.M(), sys);
    m_Fbranches.at(prefix + "pt_bb").set(*event, H_bb_uncorr.Pt(), sys);
    m_Fbranches.at(prefix + "eta_bb").set(*event, H_bb_uncorr.Eta(), sys);
    m_Fbranches.at(prefix + "phi_bb").set(*event, H_bb_uncorr.Phi(), sys);
    m_Fbranches.at(prefix + "dR_bb").set(*event, ROOT::Math::VectorUtil::DeltaR(m_jet1_uncorr, m_jet2_uncorr), sys);
    m_Fbranches.at(prefix + "Jet1_pt").set(*event, m_jet1_uncorr.Pt(), sys);
    m_Fbranches.at(prefix + "Jet1_eta").set(*event, m_jet1_uncorr.Eta(), sys);
    m_Fbranches.at(prefix + "Jet1_phi").set(*event, m_jet1_uncorr.Phi(), sys);
    m_Fbranches.at(prefix + "Jet1_m").set(*event, m_jet1_uncorr.M(), sys);
    m_Fbranches.at(prefix + "Jet2_pt").set(*event, m_jet2_uncorr.Pt(), sys);
    m_Fbranches.at(prefix + "Jet2_eta").set(*event, m_jet2_uncorr.Eta(), sys);
    m_Fbranches.at(prefix + "Jet2_phi").set(*event, m_jet2_uncorr.Phi(), sys);
    m_Fbranches.at(prefix + "Jet2_m").set(*event, m_jet2_uncorr.M(), sys);

    // MuonCorr only
    xAOD::JetFourMom_t H_bb_muonCorr = m_jet1_muonCorr + m_jet2_muonCorr;
    prefix = "HbbCand_muonCorr_";
    m_Fbranches.at(prefix + "m_bb").set(*event, H_bb_muonCorr.M(), sys);
    m_Fbranches.at(prefix + "pt_bb").set(*event, H_bb_muonCorr.Pt(), sys);
    m_Fbranches.at(prefix + "eta_bb").set(*event, H_bb_muonCorr.Eta(), sys);
    m_Fbranches.at(prefix + "phi_bb").set(*event, H_bb_muonCorr.Phi(), sys);
    m_Fbranches.at(prefix + "dR_bb").set(*event, ROOT::Math::VectorUtil::DeltaR(m_jet1_muonCorr, m_jet2_muonCorr), sys);
    m_Fbranches.at(prefix + "Jet1_pt").set(*event, m_jet1_muonCorr.Pt(), sys);
    m_Fbranches.at(prefix + "Jet1_eta").set(*event, m_jet1_muonCorr.Eta(), sys);
    m_Fbranches.at(prefix + "Jet1_phi").set(*event, m_jet1_muonCorr.Phi(), sys);
    m_Fbranches.at(prefix + "Jet1_m").set(*event, m_jet1_muonCorr.M(), sys);
    m_Fbranches.at(prefix + "Jet2_pt").set(*event, m_jet2_muonCorr.Pt(), sys);
    m_Fbranches.at(prefix + "Jet2_eta").set(*event, m_jet2_muonCorr.Eta(), sys);
    m_Fbranches.at(prefix + "Jet2_phi").set(*event, m_jet2_muonCorr.Phi(), sys);
    m_Fbranches.at(prefix + "Jet2_m").set(*event, m_jet2_muonCorr.M(), sys);

    // MuonCorr + PTReco
    TLorentzVector H_bb = jet_tlvs[0] + jet_tlvs[1];
    prefix = "HbbCand_PtCorr_";
    m_Fbranches.at(prefix + "m_bb").set(*event, H_bb.M(), sys);
    m_Fbranches.at(prefix + "pt_bb").set(*event, H_bb.Pt(), sys);
    m_Fbranches.at(prefix + "eta_bb").set(*event, H_bb.Eta(), sys);
    m_Fbranches.at(prefix + "phi_bb").set(*event, H_bb.Phi(), sys);
    m_Fbranches.at(prefix + "dR_bb").set(*event, jet_tlvs[0].DeltaR(jet_tlvs[1]), sys);
    m_Fbranches.at(prefix + "Jet1_pt").set(*event, jet_tlvs[0].Pt(), sys);
    m_Fbranches.at(prefix + "Jet1_eta").set(*event, jet_tlvs[0].Eta(), sys);
    m_Fbranches.at(prefix + "Jet1_phi").set(*event, jet_tlvs[0].Phi(), sys);
    m_Fbranches.at(prefix + "Jet1_m").set(*event, jet_tlvs[0].M(), sys);
    m_Fbranches.at(prefix + "Jet2_pt").set(*event, jet_tlvs[1].Pt(), sys);
    m_Fbranches.at(prefix + "Jet2_eta").set(*event, jet_tlvs[1].Eta(), sys);
    m_Fbranches.at(prefix + "Jet2_phi").set(*event, jet_tlvs[1].Phi(), sys);
    m_Fbranches.at(prefix + "Jet2_m").set(*event, jet_tlvs[1].M(), sys);

    // Add kinematic fit information if available
    TLorentzVector H_bb_KF = KFJets[0]->p4() + KFJets[1]->p4();
    prefix = "HbbCand_KF_";
    
    // Cache KF p4 calls as well
    TLorentzVector KF_jet1_tlv = KFJets[0]->p4();
    TLorentzVector KF_jet2_tlv = KFJets[1]->p4();
    
    m_Fbranches.at("HbbCand_KF_m_bb_unconstrained").set(*event, m_KF_MBB.get(*event, sys), sys);
    m_Fbranches.at(prefix + "m_bb").set(*event, H_bb_KF.M(), sys);
    m_Fbranches.at(prefix + "pt_bb").set(*event, H_bb_KF.Pt(), sys);
    m_Fbranches.at(prefix + "eta_bb").set(*event, H_bb_KF.Eta(), sys);
    m_Fbranches.at(prefix + "phi_bb").set(*event, H_bb_KF.Phi(), sys);
    m_Fbranches.at(prefix + "dR_bb").set(*event, KF_jet1_tlv.DeltaR(KF_jet2_tlv), sys);
    m_Fbranches.at(prefix + "Jet1_pt").set(*event, KF_jet1_tlv.Pt(), sys);
    m_Fbranches.at(prefix + "Jet1_eta").set(*event, KF_jet1_tlv.Eta(), sys);
    m_Fbranches.at(prefix + "Jet1_phi").set(*event, KF_jet1_tlv.Phi(), sys);
    m_Fbranches.at(prefix + "Jet1_m").set(*event, KF_jet1_tlv.M(), sys);
    m_Fbranches.at(prefix + "Jet2_pt").set(*event, KF_jet2_tlv.Pt(), sys);
    m_Fbranches.at(prefix + "Jet2_eta").set(*event, KF_jet2_tlv.Eta(), sys);
    m_Fbranches.at(prefix + "Jet2_phi").set(*event, KF_jet2_tlv.Phi(), sys);
    m_Fbranches.at(prefix + "Jet2_m").set(*event, KF_jet2_tlv.M(), sys);

    return StatusCode::SUCCESS;
}

}
