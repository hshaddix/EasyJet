/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/
/// @author Kira Abeling, JaeJin Hong
#include "BaselineVarsbbVVAlg.h"
#include <AthContainers/ConstDataVector.h>

#include "TLorentzVector.h"

namespace HHBBVV
{  
  StatusCode BaselineVarsbbVVAlg::initialize()
  {
    // Read syst-aware input handles
    ATH_CHECK (m_bbVVJetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_bbVVLRJetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_bbVVElectronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_bbVVMuonHandle.initialize(m_systematicsList));

    ATH_CHECK (m_bbVVTauHandle.initialize(m_systematicsList));

    ATH_CHECK (m_metHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    // SF access
    if(m_isMC){
      ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
      m_ele_SF = CP::SysReadDecorHandle<float>("effSF_"+m_eleWPName+"_%SYS%", this);
      ATH_CHECK (m_ele_SF.initialize(m_systematicsList, m_electronHandle));
      
      ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
      m_mu_SF = CP::SysReadDecorHandle<float>("effSF_"+m_muWPName+"_%SYS%", this);
      ATH_CHECK (m_mu_SF.initialize(m_systematicsList, m_muonHandle));
      
      if (!m_tauHandle.empty()) {
        ATH_CHECK (m_tauHandle.initialize(m_systematicsList));
        m_tau_effSF = CP::SysReadDecorHandle<float>("effSF_"+m_tauWPName+"_%SYS%", this);
        ATH_CHECK (m_tau_effSF.initialize(m_systematicsList, m_tauHandle));
      }
    }
    
    ATH_CHECK (m_selected_el.initialize(m_systematicsList, m_bbVVElectronHandle));
    ATH_CHECK (m_selected_mu.initialize(m_systematicsList, m_bbVVMuonHandle));
      
    ATH_CHECK (m_matched_el.initialize(m_systematicsList, m_bbVVElectronHandle));
    ATH_CHECK (m_matched_mu.initialize(m_systematicsList, m_bbVVMuonHandle));
    
    ATH_CHECK (m_selected_tau.initialize(m_systematicsList, m_bbVVTauHandle));
    
    
    ATH_CHECK(m_Hbb.initialize(m_systematicsList, m_bbVVLRJetHandle)); // Hbb jet
    ATH_CHECK(m_Whad.initialize(m_systematicsList, m_bbVVLRJetHandle)); // Whad jet
    ATH_CHECK(m_Whad2.initialize(m_systematicsList, m_bbVVLRJetHandle)); // Whad2 jet

    m_use_GN2X = !m_GN2X_wps.value().empty();
    m_use_GN3X = !m_GN3X_wps.value().empty();

    for(const auto& wp: m_GN2X_wps) m_GN2X_wp_Handles.emplace_back("xbb_select_GN2Xv01_" + wp, this);
    for(auto& handle : m_GN2X_wp_Handles) ATH_CHECK(handle.initialize(m_systematicsList, m_bbVVLRJetHandle));

    for(const auto& wp: m_GN3X_wps) m_GN3X_wp_Handles.emplace_back("xbb_select_GN3XPV01_hbb_" + wp, this);
    for(auto& handle : m_GN3X_wp_Handles) ATH_CHECK(handle.initialize(m_systematicsList, m_bbVVLRJetHandle));

    m_WTag_score = CP::SysReadDecorHandle<float>
      (m_WTag_Type.value()+m_WTag_WP.value()+"Tagger_Score", this);
    m_Pass_WTag = CP::SysReadDecorHandle<char>
      (m_WTag_Type.value()+m_WTag_WP.value()+"Tagger_Tagged", this);
    ATH_CHECK(m_WTag_score.initialize(m_systematicsList, m_bbVVLRJetHandle));
    ATH_CHECK(m_Pass_WTag.initialize(m_systematicsList, m_bbVVLRJetHandle));

    for(const auto& decoration : m_WTag_score_decorations) m_WTag_score_handles.emplace_back(decoration, this);
    for(auto& handle : m_WTag_score_handles) ATH_CHECK(handle.initialize(m_systematicsList, m_bbVVLRJetHandle));

    for(unsigned int i=1; i<5; i++)
      m_tau_wta.emplace_back("Tau"+std::to_string(i)+"_wta", this);
    for(auto& handle : m_tau_wta)
      ATH_CHECK(handle.initialize(m_systematicsList, m_bbVVLRJetHandle));

    for(unsigned int i=1; i<4; i++)
      m_ecf.emplace_back("ECF"+std::to_string(i), this);
    for(auto& handle : m_ecf)
      ATH_CHECK(handle.initialize(m_systematicsList, m_bbVVLRJetHandle));

    if(m_use_GN2X){
      for(const auto& decoration : m_GN2X_score_decorations) m_GN2X_score_handles.emplace_back(decoration, this);
      for(auto& handle : m_GN2X_score_handles) ATH_CHECK(handle.initialize(m_systematicsList, m_bbVVLRJetHandle));
    }

    if(m_use_GN3X){
      for(const auto& decoration : m_GN3X_score_decorations) m_GN3X_score_handles.emplace_back(decoration, this);
      for(auto& handle : m_GN3X_score_handles) ATH_CHECK(handle.initialize(m_systematicsList, m_bbVVLRJetHandle));
    }

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
    };

    for(const std::string &channel : m_channel_names){
      if (channel.std::string::find("1Lep") != std::string::npos)m_run_lep = true;
      if (channel.std::string::find("VBF") != std::string::npos) m_bbVV_VBF = true;

      if( channel == "Boosted1Lep") m_channels.push_back(HHBBVV::Boosted1Lep);
      else if ( channel == "SplitBoosted1Lep") m_channels.push_back(HHBBVV::SplitBoosted1Lep);
      else if( channel == "Boosted0Lep") m_channels.push_back(HHBBVV::Boosted0Lep);
      else if ( channel == "SplitBoosted0Lep") m_channels.push_back(HHBBVV::SplitBoosted0Lep);
      else if ( channel == "Common0Lep") m_channels.push_back(HHBBVV::Common0Lep);
      else if ( channel == "VBFBoosted1Lep") m_channels.push_back(HHBBVV::VBFboosted1Lep);
      else{
        ATH_MSG_ERROR("Unknown channel: "
          << channel << std::endl
          << "Available are: [\"Boosted1Lep\", \"SplitBoosted1Lep\", \"Boosted0Lep\", \"SplitBoosted0lep\", \"Common0lep\", \"VBFBoosted1Lep\"]");
        return StatusCode::FAILURE;
      }
      ATH_MSG_DEBUG("Running Channel: " << channel);
    };

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());    

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsbbVVAlg::execute(const EventContext& ctx)
  {
    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_bbVVJetHandle.retrieve (jets, sys, ctx));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_bbVVLRJetHandle.retrieve (lrjets, sys, ctx));

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK (m_bbVVMuonHandle.retrieve (muons, sys, ctx));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK (m_bbVVElectronHandle.retrieve (electrons, sys, ctx));

      const xAOD::TauJetContainer *taus = nullptr;
      if(m_bbVV_VBF){
        ANA_CHECK (m_bbVVTauHandle.retrieve (taus, sys, ctx));
      }

      
      const xAOD::MissingETContainer *metCont = nullptr;
      ANA_CHECK (m_metHandle.retrieve (metCont, sys, ctx));
      const xAOD::MissingET* met = (*metCont)["Final"];
      if (!met) {
          ATH_MSG_ERROR("Could not retrieve MET");
          return StatusCode::FAILURE;
      }

      for (const std::string &string_var: m_floatVariables) { // Initialize
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }
      
      for (const std::string &string_var: m_intVariables) {
        m_Ibranches.at(string_var).set(*event, -99, sys);
      }


      // Calculate vars

      TLorentzVector signal_lepton;
      float signal_lepton_SF = -99.;
      int signal_lepton_charge = -99;
      int signal_lepton_id = -99;
      //int signal_lepton_match = -99;
      const xAOD::TauJet* tau0 = nullptr;

      for(const xAOD::Electron* electron : *electrons) {
        if (m_selected_el.get(*electron, sys)){
          signal_lepton = electron->p4();
          if(m_isMC){signal_lepton_SF = m_ele_SF.get(*electron, sys);}
          signal_lepton_charge = electron->charge();
          signal_lepton_id = signal_lepton_charge > 0 ? -11 : 11;
          break; // At most one lepton selected
            }
      }
      for(const xAOD::Muon* muon : *muons) {
        if (m_selected_mu.get(*muon, sys)){
          signal_lepton = muon->p4();
          if(m_isMC){signal_lepton_SF = m_mu_SF.get(*muon, sys);}
          signal_lepton_charge = muon->charge();
          signal_lepton_id = signal_lepton_charge > 0 ? -13 : 13;
          break; 
        }
      }

      if(m_run_lep){ // Fill in the selected lepton branches
        m_Fbranches.at("Lepton_pt").set(*event, signal_lepton.Pt(), sys);
        m_Fbranches.at("Lepton_eta").set(*event, signal_lepton.Eta(), sys);
        m_Fbranches.at("Lepton_phi").set(*event, signal_lepton.Phi(), sys);
        m_Fbranches.at("Lepton_E").set(*event, signal_lepton.E(), sys);
        if(m_isMC){m_Fbranches.at("Lepton_effSF").set(*event, signal_lepton_SF, sys);}
        m_Ibranches.at("Lepton_charge").set(*event, signal_lepton_charge, sys);
        m_Ibranches.at("Lepton_pdgid").set(*event, signal_lepton_id, sys);
        
      }
      // VBF Boosted Triggermatched Leptons and Tau's
      for(const std::string &channel : m_channel_names){
          if (channel == "VBFBoosted1Lep"){
            for(const xAOD::TauJet* tau : *taus) {
                if(m_selected_tau.get(*tau, sys) && !tau0) tau0 = tau;
                else{
                    break;
                }
            }
          }
      }
      
      for(const xAOD::Jet* lrjet : *lrjets)
      {
        std::string prefix = "";
        if (m_Whad.get(*lrjet, sys))
        {
          prefix = "Whad_Jet";
          if(m_run_lep){
            m_Fbranches.at(prefix+"_DeltaR").set(*event, lrjet->p4().DeltaR(signal_lepton), sys);
          }

          for(auto channel: m_channels){
            if(channel == HHBBVV::Boosted0Lep){
              for (int i = 0; i < 4; i++){
                float wta_value = m_tau_wta.at(i).get(*lrjet, sys);
                wta_value = (wta_value < 1e-8) ? -99. : wta_value ;
                m_Fbranches.at(prefix + "_Tau" + std::to_string(i + 1) + "_wta").set(*event, wta_value, sys);
              }

              for (int i = 0; i < 3; i++){
                float ecf_value = m_ecf.at(i).get(*lrjet, sys);
                ecf_value = (ecf_value < 1e-8) ? -99. : ecf_value ;
                m_Fbranches.at(prefix + "_ECF" + std::to_string(i + 1)).set(*event, ecf_value, sys);
              }
            }
          }
        }
        else if (m_Hbb.get(*lrjet, sys)){
          prefix = "Hbb_Jet";
        }
        else{
          for(auto channel: m_channels){
             if(channel == HHBBVV::SplitBoosted0Lep){
               if (m_Whad2.get(*lrjet, sys)) prefix = "Whad2_Jet";
             }
          }
        }


        if(!prefix.empty()){
          m_Fbranches.at(prefix + "_pt").set(*event, lrjet->pt(), sys);
          m_Fbranches.at(prefix + "_eta").set(*event, lrjet->eta(), sys);
          m_Fbranches.at(prefix + "_phi").set(*event, lrjet->phi(), sys);
          m_Fbranches.at(prefix + "_m").set(*event, lrjet->m(), sys);

          float wtag_score = m_WTag_score.get(*lrjet, sys);
          if(m_use_GN2X){
            for(unsigned int score=0; score<m_GN2X_score_handles.size(); score++)
            {
              float value = m_GN2X_score_handles.at(score).get(*lrjet, sys);
              m_Fbranches.at(prefix+"_"+m_GN2X_score_decorations[score]).set(
                *event, value, sys);
            }
          }
          if(m_use_GN3X){
            for(unsigned int score=0; score<m_GN3X_score_handles.size(); score++)
            {
              float value = m_GN3X_score_handles.at(score).get(*lrjet, sys);
              m_Fbranches.at(prefix+"_"+m_GN3X_score_decorations[score]).set(
                *event, value, sys);
            }
          }
          m_Fbranches.at(prefix+"_"+m_WTag_Type+"_"+m_WTag_WP+"_Score").set(*event, wtag_score, sys);

          for(unsigned int score=0; score<m_WTag_score_handles.size(); score++)
          {
            float value = m_WTag_score_handles.at(score).get(*lrjet, sys);
            m_Fbranches.at(prefix+"_"+m_WTag_score_decorations[score]).set(*event, value, sys);
          }

          int pass_wtag = static_cast<int>(m_Pass_WTag.get(*lrjet, sys));
          m_Ibranches.at(prefix+"_Pass_"+m_WTag_Type+"_"+m_WTag_WP).set(*event, pass_wtag, sys);

          for(unsigned int wp=0; wp<m_GN2X_wps.size(); wp++)
          {
            int pass_GN2X = m_GN2X_wp_Handles.at(wp).get(*lrjet, sys);
            m_Ibranches.at(
              prefix+"_Pass_GN2X_"+m_GN2X_wps[wp]
            ).set(*event, pass_GN2X, sys);
          }

          for(unsigned int wp=0; wp<m_GN3X_wps.size(); wp++)
          {
            int pass_GN3X = m_GN3X_wp_Handles.at(wp).get(*lrjet, sys);
            m_Ibranches.at(
              prefix+"_Pass_GN3XPV01_hbb_"+m_GN3X_wps[wp]
            ).set(*event, pass_GN3X, sys);
          }
        }
      }


      m_Ibranches.at("lrjets_n").set(*event, lrjets->size(), sys);
      if(m_run_lep && !m_bbVV_VBF){
          m_Ibranches.at("srjets_n").set(*event, jets->size(), sys);
          m_Ibranches.at("Selected_Lepton_n").set(*event, (electrons->size()+muons->size()), sys);      
      }
      // DiHiggs mass 
      TLorentzVector bb(0,0,0,0);
      TLorentzVector VV(0,0,0,0);
      TLorentzVector HH(0,0,0,0);
      TLorentzVector HH_vis(0,0,0,0);
      TLorentzVector HH_visMet(0,0,0,0);

      // TODO: implement bbVV variables: Hbb, Whad, HH system
    }

    return StatusCode::SUCCESS;
  }
}
