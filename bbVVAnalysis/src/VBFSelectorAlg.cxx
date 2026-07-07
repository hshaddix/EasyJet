/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// @author Celine Stauch

#include "VBFSelectorAlg.h"
#include <AthenaKernel/Units.h>

#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthContainers/ConstDataVector.h>

namespace HHBBVV
{
  StatusCode VBFSelectorAlg::initialize()
  {

    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));

    // Read syst-aware input handles
    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
      
    ATH_CHECK (m_tauHandle.initialize(m_systematicsList));
    
    // Choose the first GN2X wp
    m_Pass_GN2X = CP::SysReadDecorHandle<int>
      ("xbb_select_GN2Xv01_" + m_GN2X_wps.value().front(), this);
    ATH_CHECK (m_Pass_GN2X.initialize(m_systematicsList, m_lrjetHandle));

    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_metHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    m_eleWPDecorHandle = CP::SysReadDecorHandle<char>
      ("baselineSelection_" + m_eleWPName+"_%SYS%", this);

    m_ele_TightTighTrackOnlyDecorHandle = CP::SysReadDecorHandle<char>
      ("baselineSelection_"+m_ele_TightTighTrackOnlyWPName+"_%SYS%", this);
    ATH_CHECK(m_ele_TightTighTrackOnlyDecorHandle.initialize(m_systematicsList, m_electronHandle));

    m_muonWPDecorHandle = CP::SysReadDecorHandle<char>
      ("baselineSelection_"+m_muonWPName+"_%SYS%", this);
    
    m_mu_TightPflowLooseDecorHandle = CP::SysReadDecorHandle<char>
      ("baselineSelection_"+m_mu_TightPflowLooseWPName+"_%SYS%", this);
    ATH_CHECK(m_mu_TightPflowLooseDecorHandle.initialize(m_systematicsList, m_muonHandle));

    if (!m_tauHandle.empty()) {
      m_tauWPDecorHandle = CP::SysReadDecorHandle<char>
        ("baselineSelection_" + m_tauWPName+"_%SYS%", this);
    }

    
    ATH_CHECK(m_eleWPDecorHandle.initialize(m_systematicsList, m_electronHandle));
    ATH_CHECK(m_muonWPDecorHandle.initialize(m_systematicsList, m_muonHandle));

    ATH_CHECK(m_tauWPDecorHandle.initialize(m_systematicsList, m_tauHandle));
    ATH_CHECK(m_selected_tau.initialize(m_systematicsList, m_tauHandle));
    
    ATH_CHECK(m_selected_el.initialize(m_systematicsList, m_electronHandle));
    ATH_CHECK(m_selected_mu.initialize(m_systematicsList, m_muonHandle));
    ATH_CHECK(m_matched_el.initialize(m_systematicsList, m_electronHandle));
    ATH_CHECK(m_matched_mu.initialize(m_systematicsList, m_muonHandle));

    // special flag for all cuts
    ATH_CHECK (m_passallcuts.initialize(m_systematicsList, m_eventHandle));
      
    ATH_CHECK(m_Whad.initialize(m_systematicsList, m_lrjetHandle));
    ATH_CHECK(m_Hbb.initialize(m_systematicsList, m_lrjetHandle));
    
    if (!m_isBtag.empty()) {
      ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_jetHandle));
    }
    ATH_CHECK(m_year.initialize(m_systematicsList, m_eventHandle));
      
    // Intialise booleans with value false.
    for (auto& [key, value] : m_boolnames) {
      m_bools.emplace(key, false);
      CP::SysWriteDecorHandle<char> whandle{value+"_%SYS%", this};
      m_Bbranches.emplace(key, whandle);
      ATH_CHECK(m_Bbranches.at(key).initialize(m_systematicsList, m_eventHandle));
    };

    // Initialise trigger decorators
    for (auto trig : m_triggers){
      CP::SysReadDecorHandle<bool> deco {trig, this};
      m_triggerdecos.emplace(trig, deco);
      ATH_CHECK(m_triggerdecos.at(trig).initialize(m_systematicsList, m_eventHandle));
    }
    // Weight for cutflow
    if (m_saveCutFlow) ATH_CHECK (m_generatorWeight.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst-aware output decorators
    ATH_CHECK(m_pass_sr.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());
      
    for ( auto name : m_channel_names){
      if ( name == "VBFBoosted1Lep") m_channels.push_back(HHBBVV::VBFboosted1Lep);
      else{
        ATH_MSG_ERROR("Unknown channel: "
          << name << std::endl
          << "Available are: [\"VBFBoosted1Lep\"]");
        return StatusCode::FAILURE;
      }
    }

    if(m_saveCutFlow) ATH_CHECK (initialiseCutflow());
    return StatusCode::SUCCESS;
  }

  StatusCode VBFSelectorAlg::execute(const EventContext& ctx)
  {
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys, ctx));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys, ctx));

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK (m_muonHandle.retrieve (muons, sys, ctx));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK (m_electronHandle.retrieve (electrons, sys, ctx));

      const xAOD::TauJetContainer *taus = nullptr;
      ANA_CHECK (m_tauHandle.retrieve (taus, sys, ctx));
      
      const xAOD::MissingETContainer *metCont = nullptr;
      ANA_CHECK (m_metHandle.retrieve (metCont, sys, ctx));
      const xAOD::MissingET* met = (*metCont)["Final"];
      if (!met) {
      	ATH_MSG_ERROR("Could not retrieve MET");
      	return StatusCode::FAILURE;
      }
        
      // Apply selection
      for (auto& [key, value] : m_boolnames) m_bools.at(key) = false;

      // reset all cut flags to default=false
      for (CutEntry& cut : m_bbVVCuts) {
        cut.passed = false;
      }
      n_evt+=1;
      if (m_saveCutFlow && m_isMC) {
        m_total_mcEventWeight+= m_generatorWeight.get(*event, sys);
      }


      // Apply selection
      // baseline selection for boosted in bbVV_boosted_config.py

      // flags
      bool TWO_LRJETS = false;
      bool ONE_LEP = false;
      bool TWOVBFJET = false;
        

      //************
      // lepton
      //************
      int n_leptons = 0;
      TLorentzVector signal_lepton;
      //example - needs to match bbVV definition
      // requirements for signal electron
      const xAOD::Electron* ele0 = nullptr;
      const xAOD::Electron* ele1 = nullptr;
      // requirements for signal electrons
      bool TightId_ele = false;
      for (const xAOD::Electron *electron : *electrons)
      {
        bool passElectronWP = m_eleWPDecorHandle.get(*electron, sys);
        m_selected_el.set(*electron, false, sys);
        m_matched_el.set(*electron, false, sys);
        if (passElectronWP)
        {
          TightId_ele = m_ele_TightTighTrackOnlyDecorHandle.get(*electron, sys);
          // give passing electron decoration "selected_el"
          m_selected_el.set(*electron, true, sys);
          n_leptons += 1;
          if(!ele0) ele0 = electron;
          else if(!ele1) ele1 = electron;

        }
      }
        
      // requirements for signal muons
      const xAOD::Muon* mu0 = nullptr;
      const xAOD::Muon* mu1 = nullptr;
      bool TightId_mu = false;
      for (const xAOD::Muon *muon : *muons)
      {
        bool passMuonWP = m_muonWPDecorHandle.get(*muon, sys);
        m_selected_mu.set(*muon, false, sys);
        m_matched_mu.set(*muon, false, sys);
        if (passMuonWP && std::abs(muon->eta()) < 2.5)
        {
          TightId_mu = m_mu_TightPflowLooseDecorHandle.get(*muon, sys);
          m_selected_mu.set(*muon, true, sys); // currently could be multiple selected per event?
          n_leptons += 1;
          if(!mu0) mu0 = muon;
          else if(!mu1) mu1 = muon;

        }
      }
      // check for exactly one lepton
      
      if (n_leptons == 1)
      {
        ONE_LEP = true;
        m_bbVVCuts("EXACTLY_ONE_LEPTON").passed = true;
        if(TightId_mu || TightId_ele) m_bbVVCuts("SIGNAL_LEPTON").passed = true;
      }
      
      //************
      // pflow jet
      //************
      bool WPgiven = !m_isBtag.empty();
      auto bjets = std::make_unique<ConstDataVector<xAOD::JetContainer>>(
          SG::VIEW_ELEMENTS);
      auto smallRjets = std::make_unique<ConstDataVector<xAOD::JetContainer>>(
          SG::VIEW_ELEMENTS);


      const xAOD::Jet *lrjet = nullptr;
      const xAOD::Jet *sublrjet = nullptr;

      for (const xAOD::Jet *jet : *jets) {
        if (jet->pt() < 20. * Athena::Units::GeV) continue;
        if (std::abs(jet->eta()) < 2.5) {
          if (WPgiven) {
            if (m_isBtag.get(*jet, sys)){
              bjets->push_back(jet);
            }else{
              smallRjets->push_back(jet);
            }
          }
        }
        else if(std::abs(jet->eta()) < 4.5) {
          smallRjets->push_back(jet);
        }
      }
      

      //************
      // large-R jet
      //************
      int n_lrjets = lrjets->size();
      float leadLRJ_pt = -999.;
       
      for (const xAOD::Jet *lrjet : *lrjets)
      {
        m_Hbb.set(*lrjet,false,sys);  
        m_Whad.set(*lrjet,false,sys);
      }  
        
      ATH_MSG_DEBUG("# lrjets: " << n_lrjets);
      if (n_lrjets >= 2)
      {
        
        if(n_lrjets == 2){
            TWO_LRJETS = true;
            m_bbVVCuts("EXACTLY_TWO_LRJETS").passed = true;
        }
        //further selections can go here
          
        leadLRJ_pt = lrjets->get(0)->pt();
        lrjet = lrjets->get(0);
        sublrjet = lrjets->get(1);

      }

      for(const auto& channel : m_channels){
        ATH_MSG_DEBUG("channel: " << channel);
        if(channel == HHBBVV::VBFboosted1Lep)
        {
          ATH_MSG_DEBUG("---VBF BOOSTED 1LEP JET CLASSIFICATION---");
          if (TWO_LRJETS && ONE_LEP)
          {
            VBFJetSelectionboosted(*jets, TWOVBFJET, lrjet, sublrjet);
          }
        }

      }


      //************
      // taujet
      //************
      int n_taus = 0;

      for (const xAOD::TauJet *tau : *taus)
        {
          bool passTauWP = m_tauWPDecorHandle.get(*tau, sys);
          m_selected_tau.set(*tau, false, sys);
           if (passTauWP && tau->pt() > 20. * Athena::Units::GeV && std::abs(tau->charge())==1 && (tau->nTracks()==1 || tau->nTracks()==3)){
            m_selected_tau.set(*tau, true, sys);
            n_taus += 1;
           }
        }
      if(n_taus == 0) m_bbVVCuts("NO_TAU").passed = true;
        
      if(m_useTriggerSel && (ele0 || mu0) && ONE_LEP){
        if(leadLRJ_pt > 520. * Athena::Units::GeV){
            evaluateLRJetTrigger(event, lrjet, sys);
        }
        else{
            evaluateSingleLeptonTrigger(event, ele0, mu0, sys);
            if (m_bools.at(HHBBVV::pass_trigger_SET)){
               for (const xAOD::Electron* electron : *electrons){
                 m_matched_el.set(*electron, true, sys);
                 break;
               }
            }
            if (m_bools.at(HHBBVV::pass_trigger_SMT)){
              for (const xAOD::Muon* muon : *muons){
                m_matched_mu.set(*muon, true, sys);
                break;
              }
            }
        }
     }
     
      m_Bbranches.at(HHBBVV::pass_trigger_SLT).set(*event, m_bools.at(HHBBVV::pass_trigger_SLT), sys);
      m_Bbranches.at(HHBBVV::pass_trigger_LRT).set(*event, m_bools.at(HHBBVV::pass_trigger_LRT), sys);
      
      bool passedall = true;
      for (CutEntry& cut : m_bbVVCuts) {
        passedall = passedall && cut.passed;
      }
      m_passallcuts.set(*event, passedall, sys); // Mark this event all passed
      if(passedall)m_bbVVCuts.PassAllCuts += 1; // Count all passed event N


      // Count how many cuts the event passed and increase the relative counter
      for (const auto &cut : m_STANDARD_CUTS)  {
        if(m_bbVVCuts.exists(cut)) {
          if (m_bbVVCuts(cut).passed)
            m_bbVVCuts(cut).counter+=1;
        }
      }

      // Check how many consecutive cuts are passed by the event.
      unsigned int consecutive_cuts = 0;
      for (size_t i = 0; i < m_bbVVCuts.size(); ++i) {
        if (m_bbVVCuts[i].passed)
          consecutive_cuts++;
        else
          break;
      }

      // Here we basically increment the  N_events(pass_i  AND pass_i-1  AND ... AND pass_0) for the i-cut.
      for (unsigned int i=0; i<consecutive_cuts; i++) {
        m_bbVVCuts[i].relativeCounter+=1;
        if (m_isMC) m_bbVVCuts[i].w_relativeCounter += m_generatorWeight.get(*event, sys);
      }



      //****************
      // event level info
      //****************
      //for example masses of the system, met, fired triggers etc
      
      bool pass = false;
      //implement dedicated selection flags here
      for(const auto& channel : m_channels){
        if(channel == HHBBVV::VBFboosted1Lep) pass |= (TWO_LRJETS && ONE_LEP && TWOVBFJET && (m_bools.at(HHBBVV::pass_trigger_SLT) || m_bools.at(HHBBVV::pass_trigger_LRT)));
      }
      ATH_MSG_DEBUG("pass:" << pass);
      ATH_MSG_DEBUG("NEXT EVENT");
      if (!m_bypass && !pass) continue; // maybe add skipped_evts += evt_weights;
      
      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      filter.setPassed(true);
    }
    
    return StatusCode::SUCCESS;
  }

  StatusCode VBFSelectorAlg::finalize() {

    m_bbVVCuts.CheckCutResults(); // Print CheckCutResults
    if(m_saveCutFlow) 
    {
      m_bbVVCuts.DoAbsoluteEfficiency(n_evt, efficiency("AbsoluteEfficiency"));
      m_bbVVCuts.DoRelativeEfficiency(n_evt, efficiency("RelativeEfficiency"));
      m_bbVVCuts.DoStandardCutFlow(n_evt, efficiency("StandardCutFlow"));
      if (m_isMC) m_bbVVCuts.DoWeightedStandardCutFlow(m_total_mcEventWeight, efficiency("WeightedStandardCutFlow"));
      m_bbVVCuts.DoCutflowLabeling(n_evt, hist("EventsPassed_BinLabeling"));
    }

    // Cut information
    ATH_MSG_DEBUG("###########cuts##########");
    ATH_MSG_DEBUG("events:" << n_evt);
    ATH_MSG_DEBUG("min. 2 jets:" << Jet_evt); // Temporary, need to decide whether bbVV needs AT_LEAST_TWO_JETS cut
    ANA_CHECK (m_filterParams.finalize ());

    return StatusCode::SUCCESS;
  }

  void VBFSelectorAlg::VBFJetSelectionboosted(const xAOD::JetContainer& jets, bool & TWOVBFJET, const xAOD::Jet *&Hbb, const xAOD::Jet *&Whad){
        ROOT::Math::PtEtaPhiMVector fwjet1_vec, fwjet2_vec;
        auto fwjets = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);

        for (const auto jet : (jets))
        {
           float DeltaR_Whad =  (Whad->p4()).DeltaR(jet->p4());
           float DeltaR_Hbb = (Hbb->p4()).DeltaR(jet->p4());

           if (DeltaR_Whad > 1.4 && DeltaR_Hbb > 1.4)
           {
             fwjets->push_back(jet);
           }
        }
        if((*fwjets).size() >= 2){
          int nfwjets = (*fwjets).size();
          for ( int i=0; i < nfwjets; i++){
              for ( int j = i; j < nfwjets; j++){
                  fwjet1_vec.SetCoordinates((*fwjets)[i]->pt(),(*fwjets)[i]->eta(),(*fwjets)[i]->phi(),(*fwjets)[i]->m());
                  fwjet2_vec.SetCoordinates((*fwjets)[j]->pt(),(*fwjets)[j]->eta(),(*fwjets)[j]->phi(),(*fwjets)[j]->m()); 
                  if( std::abs(fwjet1_vec.Eta()-fwjet2_vec.Eta()) > 3.5 ){
                      TWOVBFJET = true;
                      break;
                  }
              }
          }    
        }
      if (TWOVBFJET) m_bbVVCuts("2VBF_JETS").passed = true;
  }

  void VBFSelectorAlg::evaluateSingleLeptonTrigger(const xAOD::EventInfo *event, const xAOD::Electron *ele, const xAOD::Muon *mu, const CP::SystematicSet& sys)
  {
    // Check single electron triggers
    std::vector<std::string> single_ele_paths;

    int year = m_year.get(*event, sys);
    if(year==2015){
      single_ele_paths = {
        "HLT_e24_lhmedium_L1EM20VH",
	"HLT_e60_lhmedium",
        "HLT_e120_lhloose"
      };
    }
    else if(year == 2016 || year==2017 || year == 2018){
      single_ele_paths = {
	"HLT_e26_lhtight_nod0_ivarloose",
	"HLT_e60_lhmedium_nod0",
	"HLT_e140_lhloose_nod0",
      };   
    }
    else if(year==2022){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1EM22VHI",
	"HLT_e60_lhmedium_L1EM22VHI",
        "HLT_e140_lhloose_L1EM22VHI"
      };
    }
    // same for 2024
    else if(year==2023 || year == 2024){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1eEM26M",
	"HLT_e60_lhmedium_L1eEM26M",
        "HLT_e140_lhloose_L1eEM26M"
      };
    }


    if(ele){
      for(const auto& trig : single_ele_paths){
        bool pass = m_triggerdecos.at("trigPassed_"+trig).get(*event, sys);
        if (pass){
          bool match = m_matchingTool->match(*ele, trig);
          m_bools.at(HHBBVV::pass_trigger_SET) |= match;
        }
      }
      m_bools.at(HHBBVV::pass_trigger_SET) &= ele->pt() > m_pt_threshold[HHBBVV::SLT][HHBBVV::ele];
    }
    // Check single muon triggers
    std::vector<std::string> single_mu_paths;

    if(year==2015){
      single_mu_paths = {
	"HLT_mu20_iloose_L1MU15",
	"HLT_mu40"
      };
    }
    else if(year == 2016 || year == 2017 || year == 2018){
      single_mu_paths = {
	"HLT_mu26_ivarmedium",
	"HLT_mu50"
      };
    }
    // same for 2024
    else if(year == 2022 || year == 2023 || year == 2024){
      single_mu_paths = {
        "HLT_mu24_ivarmedium_L1MU14FCH",
	"HLT_mu50_L1MU14FCH"
      };
    }


    if (mu){
      for(const auto& trig : single_mu_paths){
        bool pass = m_triggerdecos.at("trigPassed_"+trig).get(*event, sys);
        if (pass){
          bool match = m_matchingTool->match(*mu, trig);
          m_bools.at(HHBBVV::pass_trigger_SMT) |= match;
        }
      }
      m_bools.at(HHBBVV::pass_trigger_SMT) &= mu->pt() > m_pt_threshold[HHBBVV::SLT][HHBBVV::mu];
    }
    if(m_bools.at(HHBBVV::pass_trigger_SET) || m_bools.at(HHBBVV::pass_trigger_SMT)){
      m_bbVVCuts("PASS_TRIGGER").passed = true;
      m_bools.at(HHBBVV::pass_trigger_SLT) |= (m_bools.at(HHBBVV::pass_trigger_SET) || m_bools.at(HHBBVV::pass_trigger_SMT));
    }
  }
  
  void VBFSelectorAlg::evaluateLRJetTrigger(const xAOD::EventInfo *event, const xAOD::Jet *lrjet, const CP::SystematicSet& sys)
  {
    std::vector<std::string> single_lr_paths;

    int year = m_year.get(*event, sys);
    if(year==2015){
      single_lr_paths = {
	"HLT_j360_a10_lcw_sub_L1J100",
	"HLT_j360_a10_sub_L1J100"
      };
    }
    else if(year == 2016){
      single_lr_paths = {
        "HLT_j360_a10r_L1J100",
	"HLT_j360_a10_lcw_L1J100",
	"HLT_j400_a10r_L1J100",
	"HLT_j400_a10_lcw_L1J100",
	"HLT_j420_a10_lcw_L1J100",
	"HLT_j420_a10r_L1J100"
      };
    }
    else if(year==2017){
      single_lr_paths = {
	"HLT_j420_a10t_lcw_jes_L1J100",
	"HLT_j440_a10t_lcw_jes_L1J100",
	"HLT_j460_a10t_lcw_jes_L1J100",
	"HLT_j480_a10t_lcw_jes_L1J100",
	"HLT_j390_a10t_lcw_jes_30smcINF_L1J100",
	"HLT_j420_a10t_lcw_jes_40smcINF_L1J100",
	"HLT_j440_a10t_lcw_jes_40smcINF_L1J100",
	"HLT_j420_a10_lcw_subjes_L1J100",
	"HLT_j440_a10_lcw_subjes_L1J100",
	"HLT_j460_a10_lcw_subjes_L1J100",
	"HLT_j480_a10_lcw_subjes_L1J100",
	"HLT_j420_a10r_L1J100",
	"HLT_j440_a10r_L1J100",
	"HLT_j460_a10r_L1J100",
	"HLT_j480_a10r_L1J100"
      };   
    }
    else if(year == 2018){
      single_lr_paths = {
	"HLT_j460_a10r_L1SC111",
	"HLT_j460_a10r_L1J100",
	"HLT_j460_a10_lcw_subjes_L1SC111",
	"HLT_j460_a10_lcw_subjes_L1J100",
	"HLT_j460_a10t_lcw_jes_L1SC111",
	"HLT_j460_a10t_lcw_jes_L1J100",
	"HLT_j420_a10t_lcw_jes_35smcINF_L1SC111",
	"HLT_j420_a10t_lcw_jes_35smcINF_L1J100"	
      };
    }
    else if(year==2022){
      single_lr_paths = {
        "HLT_j460_a10sd_cssk_pf_jes_ftf_preselj225_L1J100",
	"HLT_j460_a10t_lcw_jes_L1J100",
	"HLT_j460_a10r_L1J100",
	"HLT_j460_a10_lcw_subjes_L1J100",
	"HLT_j420_35smcINF_a10sd_cssk_pf_jes_ftf_preselj225_L1J100",
	"HLT_j420_35smcINF_a10t_lcw_jes_L1J100"
      };
    }
    else if(year==2023){
      single_lr_paths = {
	"HLT_j460_a10sd_cssk_pf_jes_ftf_preselj225_L1J100",
	"HLT_j460_a10t_lcw_jes_L1J100",
	"HLT_j460_a10r_L1J100",
	"HLT_j460_a10_lcw_subjes_L1J100",
	"HLT_j420_35smcINF_a10sd_cssk_pf_jes_ftf_preselj225_L1J100",
	"HLT_j420_35smcINF_a10t_lcw_jes_L1J100"
      };
    }
    else if(year==2024){
      single_lr_paths = {
	"HLT_j360_a10t_lcw_jes_L1jJ160",
    "HLT_j420_a10sd_cssk_pf_jes_ftf_preselj225_L1jJ160",
    "HLT_j460_a10sd_cssk_pf_jes_ftf_preselj225_L1jJ160",
    "HLT_j460_a10sd_cssk_pf_jes_ftf_preselj225_L1gLJ140p0ETA25",
    "HLT_j460_a10t_lcw_jes_L1jJ160",
    "HLT_j460_a10r_L1jJ160",
    "HLT_j460_a10_lcw_subjes_L1jJ160"
      };
    }
    bool pass = false;
    if(lrjet){
    for(const auto& trig : single_lr_paths){
         pass = m_triggerdecos.at("trigPassed_"+trig).get(*event, sys);
      }
    }
        m_bbVVCuts("PASS_TRIGGER").passed = pass;
        m_bools.at(HHBBVV::pass_trigger_LRT) |= pass;
  }

  void VBFSelectorAlg::setThresholds(const xAOD::EventInfo* event,const CP::SystematicSet& sys) {
    
    int year = m_year.get(*event, sys);

    // Single-lepton triggers
    if(year==2015)
      m_pt_threshold[HHBBVV::SLT][HHBBVV::ele] = 25. * Athena::Units::GeV;
    else
      m_pt_threshold[HHBBVV::SLT][HHBBVV::ele] = 27. * Athena::Units::GeV;

    if(year==2015)
      m_pt_threshold[HHBBVV::SLT][HHBBVV::mu] = 21. * Athena::Units::GeV;
    else if(year>=2016 && year<=2018)
      m_pt_threshold[HHBBVV::SLT][HHBBVV::mu] = 27. * Athena::Units::GeV;
    else
      m_pt_threshold[HHBBVV::SLT][HHBBVV::mu] = 25. * Athena::Units::GeV;
  }

    
  StatusCode VBFSelectorAlg::initialiseCutflow(){
    m_bbVVCuts.CheckInputCutList(m_inputCutList,m_STANDARD_CUTS); // Check our inputCutList is valid

    for (const std::string &cut : m_inputCutList)  {
      // Initialize a vector of CutEntry structs based on the input Cut List
      m_bbVVCuts.add(cut);
      ATH_MSG_DEBUG("bbVVCut = " << cut);
    }

    //After filling the CutManager, book your histograms.
    const unsigned int nbins = m_bbVVCuts.size() + 1; //  need an extra bin for the total num of events.
    ANA_CHECK (book (TEfficiency("AbsoluteEfficiency","Absolute Efficiency of HH->bbVV cuts;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TEfficiency("RelativeEfficiency","Relative Efficiency of HH->bbVV cuts;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TEfficiency("StandardCutFlow","StandardCutFlow of HH->bbVV cuts;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
    if (m_isMC) ANA_CHECK (book (TEfficiency("WeightedStandardCutFlow","Weighted StandardCutFlow of HH->bbVV cuts. Needs rescaling to sumOfWeights.;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TH1F("EventsPassed_BinLabeling", "Events passed by each cut / Bin labeling", nbins, 0.5, nbins + 0.5)));

    return StatusCode::SUCCESS;
  }

}

