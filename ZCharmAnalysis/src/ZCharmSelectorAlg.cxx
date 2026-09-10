/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZCharmSelectorAlg.h"
#include <AthenaKernel/Units.h>

#include <AsgDataHandles/ReadDecorHandle.h>
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthContainers/ConstDataVector.h>


namespace ZCC
{
  StatusCode ZCharmSelectorAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     ZCharmSelectorAlg      \n");
    ATH_MSG_INFO("*********************************\n");

    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    
    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));

    for (const std::string &var : m_PCBTnames){
      ATH_MSG_DEBUG("initializing PCBT: " << var);
      CP::SysReadDecorHandle<int> rhandle{var, this};
      m_PCBTs.emplace(var, rhandle);
      ATH_CHECK(m_PCBTs.at(var).initialize(m_systematicsList, m_jetHandle));
    };

    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    if (m_saveCutFlow) ATH_CHECK (m_generatorWeight.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK(m_year.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK(m_is17_periodB5_B8.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_is22_75bunches.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_is23_75bunches.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_is23_400bunches.initialize(m_systematicsList, m_eventHandle));
    
    ATH_CHECK (m_matchingTool.retrieve());

    ATH_CHECK(m_passTruthCutsKey.initialize(m_isMC));


    for (auto& [key, value] : m_boolnames) {
      m_bools.emplace(key, false);
      CP::SysWriteDecorHandle<char> whandle{value+"_%SYS%", this};
      m_Bbranches.emplace(key, whandle);
      ATH_CHECK(m_Bbranches.at(key).initialize(m_systematicsList, m_eventHandle));
    }

    // make trigger decorators
    for (auto trig : m_triggers){
      CP::SysReadDecorHandle<bool> deco {trig, this};
      m_triggerdecos.emplace(trig, deco);
      ATH_CHECK(m_triggerdecos.at(trig).initialize(m_systematicsList, m_eventHandle));
    }

    // special flag for all cuts
    ATH_CHECK (m_pass_cuts.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    if(m_saveCutFlow){
      std::vector<std::string> boolNameList;
      for (const auto& [key, value]: m_boolnames){
        boolNameList.push_back(value);
        m_nameToBool[value] = key;
      }
      ATH_CHECK (initialiseCutflow(boolNameList));
    }

    return StatusCode::SUCCESS;
  }


  StatusCode ZCharmSelectorAlg::execute(const EventContext& ctx)
  {
    
    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ATH_CHECK (m_eventHandle.retrieve (event, sys, ctx));
                
      char pass_truth_baseline = false;
      if (m_isMC){
        // Find truth decision
        SG::ReadDecorHandle<xAOD::EventInfo, char> m_passTruthCuts(m_passTruthCutsKey, ctx);        // Retrieve the truth decision
        if (!m_passTruthCuts.isPresent()) {
          ATH_MSG_ERROR("PassTruthCuts decision is empty or not found!");
          return StatusCode::FAILURE;
        }
        pass_truth_baseline = m_passTruthCuts(*event);
      }
      
      const xAOD::JetContainer *jets = nullptr;
      ATH_CHECK (m_jetHandle.retrieve (jets, sys, ctx));

      std::string ftag2D_WP = "ftag_quantile_GN2v01_Continuous2D";
      std::vector<int> ctag_values = {1,2,3};
      std::vector<int> btag_values = {4,5,6};
      auto cjets = std::make_unique<ConstDataVector<xAOD::JetContainer>> (SG::VIEW_ELEMENTS);
      
      bool has_ftag2D = std::find(m_PCBTnames.begin(), m_PCBTnames.end(), ftag2D_WP) != m_PCBTnames.end();
      
      if(has_ftag2D)
      {
        for(const xAOD::Jet* jet : *jets) {
          int pcbt = m_PCBTs.at(ftag2D_WP).get(*jet, sys);
          bool is_ctag = std::find(ctag_values.begin(), ctag_values.end(), pcbt) != ctag_values.end();
          bool y_cut = std::abs(jet->rapidity()) < 2.5;
          if (is_ctag && y_cut) {
            cjets->push_back(jet);
          }
        }
      }

      const xAOD::MuonContainer *muons = nullptr;
      ATH_CHECK (m_muonHandle.retrieve (muons, sys, ctx));
      
      const xAOD::ElectronContainer *electrons = nullptr;
      ATH_CHECK (m_electronHandle.retrieve (electrons, sys, ctx));
      
      m_bools.at(ZCC::IS_ee) = false;
      m_bools.at(ZCC::IS_mm) = false;
      m_bools.at(ZCC::IS_em) = false;

      m_bools.at(ZCC::pass_trigger_SLT) = false;

      m_bools.at(ZCC::PASS_TRIGGER) = false;
      m_bools.at(ZCC::EXACTLY_TWO_LEPTONS) = false;
      m_bools.at(ZCC::OPPOSITE_CHARGE_LEPTONS) = false;
      m_bools.at(ZCC::DILEPTON_MASS_WINDOW) = false;

      m_bools.at(ZCC::ONE_C_JETS) = false;
      m_bools.at(ZCC::TWO_C_JETS) = false;

      setThresholds(event, sys);

      // Leptons
      const xAOD::Electron* ele0 = nullptr;
      const xAOD::Electron* ele1 = nullptr;
      
      const xAOD::Muon* mu0 = nullptr;
      const xAOD::Muon* mu1 = nullptr;
      
      if (electrons->size() >= 2) {
        ele0 = electrons->at(0);
        ele1 = electrons->at(1);
      }
      
      if (muons->size() >= 2) {
        mu0 = muons->at(0);
        mu1 = muons->at(1);
      }
      
      if (electrons->size() == 1 && muons->size() == 1) {
        ele0 = electrons->at(0);
        mu0 = muons->at(0);
      }

      evaluateTriggerCuts(event, ele0, ele1, mu0, mu1, sys);

      evaluateLeptonCuts(*electrons, *muons);
      
      evaluateCJetCuts(*cjets);
      
      bool pass_baseline=true;
      for (const auto& [key, value] : m_boolnames) {
        auto it = std::find(m_BASELINE_CUTS.begin(), m_BASELINE_CUTS.end(), value);
        if (it != m_BASELINE_CUTS.end()) {
          pass_baseline &= m_bools.at(key);
        }
      }
      pass_baseline &= (jets->size() >= 1);
      m_pass_cuts.set(*event, pass_baseline, sys);

      if (sys.name()=="" && m_saveCutFlow){
        std::vector<bool> cutBools;
        for(const auto& cut : m_inputCutList)
          cutBools.push_back(m_bools.at(m_nameToBool.at(cut)));
        double mc_weight = m_isMC ? m_generatorWeight.get(*event, sys) : 0.;
        ATH_CHECK (applyCutFlow(cutBools, mc_weight));
      }

      for (auto& [key, var] : m_bools) {
        m_Bbranches.at(key).set(*event, var, sys);
      }

      if (pass_baseline != pass_truth_baseline) {
        ATH_MSG_VERBOSE("Baseline cuts do not match truth baseline cuts! "
                        << "pass_baseline: " << pass_baseline
                        << " pass_truth_baseline: " << pass_truth_baseline);
      }

      if (!m_bypass && (!pass_baseline && !pass_truth_baseline )) {
        continue;
      }
            
      ATH_MSG_VERBOSE("Saving event number: " << event->eventNumber() << " in run: " << event->runNumber());
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZCharmSelectorAlg::finalize()
  {
    //adapt the following for each syst TODO
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);
    ATH_CHECK (m_filterParams.finalize ());
    ATH_CHECK (finaliseCutflow());

    return StatusCode::SUCCESS;
  }

  void ZCharmSelectorAlg::evaluateTriggerCuts
  (const xAOD::EventInfo *event,
   const xAOD::Electron* ele0, const xAOD::Electron* ele1,
   const xAOD::Muon* mu0, const xAOD::Muon* mu1,
   const CP::SystematicSet& sys) {
    
    if (ele0 || mu0) {
      evaluateSingleLeptonTrigger(event, ele0, mu0, sys);
    }
    
    if (ele1 || mu1) {
      evaluateSingleLeptonTrigger(event, ele1, mu1, sys);
    }

    if (m_bools.at(ZCC::pass_trigger_SLT)) {
      m_bools.at(ZCC::PASS_TRIGGER) = true;
    }

  }

  void ZCharmSelectorAlg::evaluateSingleLeptonTrigger
  (const xAOD::EventInfo *event,
   const xAOD::Electron *ele, const xAOD::Muon *mu,
   const CP::SystematicSet& sys)
  {
    // Check single electron triggers
    std::vector<std::string> single_ele_paths;

    int year = m_year.get(*event, sys);
    
    if(year==2015){
      single_ele_paths = {
        "HLT_e24_lhmedium_L1EM20VH", "HLT_e60_lhmedium",
        "HLT_e120_lhloose"
      };
    }
    else if(2016<=year && year<=2018){
      single_ele_paths = {
        "HLT_e26_lhtight_nod0_ivarloose", "HLT_e60_lhmedium_nod0",
        "HLT_e140_lhloose_nod0"
      };
    }
    else if(m_is22_75bunches.get(*event, sys)){
      single_ele_paths = {
        "HLT_e17_lhvloose_L1EM15VHI", "HLT_e20_lhvloose_L1EM15VH",
        "HLT_e250_etcut_L1EM22VHI"
      };
    }
    else if(year==2022){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1EM22VHI", "HLT_e60_lhmedium_L1EM22VHI",
        "HLT_e140_lhloose_L1EM22VHI", "HLT_e300_etcut_L1EM22VHI"
      };
    }
    else if(m_is23_75bunches.get(*event, sys)){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1EM22VHI", "HLT_e60_lhmedium_L1EM22VHI",
        "HLT_e140_lhloose_L1EM22VHI", "HLT_e140_lhloose_noringer_L1EM22VHI",
        "HLT_e300_etcut_L1EM22VHI"
      };
    }
    else if(year==2023){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1eEM26M", "HLT_e60_lhmedium_L1eEM26M",
        "HLT_e140_lhloose_L1eEM26M", "HLT_e140_lhloose_noringer_L1eEM26M",
        "HLT_e300_etcut_L1eEM26M"
      };
    }
    else if(year==2024){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1eEM26M", "HLT_e60_lhmedium_L1eEM26M",
        "HLT_e140_lhloose_L1eEM26M", "HLT_e140_lhloose_noringer_L1eEM26M",
        "HLT_e300_etcut_L1eEM26M"
      };
    }

    bool trigPassed_SET = false;
    if(ele){
      for(const auto& trig : single_ele_paths){
        bool pass = m_triggerdecos.at("trigPassed_"+trig).get(*event, sys);
        if (pass){
          bool match = m_matchingTool->match(*ele, trig);
          trigPassed_SET |= match;
        }
      }
      double pt_threshold = m_pt_threshold[ZCC::SLT][ZCC::ele];
      trigPassed_SET &= ele->pt() > pt_threshold;
    }

    // Check single muon triggers
    std::vector<std::string> single_mu_paths;

    if(year==2015){
      single_mu_paths = {"HLT_mu20_iloose_L1MU15", "HLT_mu50"};
    }
    else if(2016<=year && year<=2018){
      single_mu_paths = {"HLT_mu26_ivarmedium", "HLT_mu50"};
    }
    else if(2022<=year && year<=2023 &&
	    !m_is22_75bunches.get(*event, sys) &&
	    !m_is23_75bunches.get(*event, sys) &&
	    !m_is23_400bunches.get(*event, sys)){
      single_mu_paths = {
        "HLT_mu24_ivarmedium_L1MU14FCH", "HLT_mu50_L1MU14FCH",
        "HLT_mu60_0eta105_msonly_L1MU14FCH", "HLT_mu60_L1MU14FCH",
        "HLT_mu80_msonly_3layersEC_L1MU14FCH"
      };
    }
    else if( year==2024 ){
      single_mu_paths = {
        "HLT_mu24_ivarmedium_L1MU14FCH", "HLT_mu50_L1MU14FCH",
        "HLT_mu60_0eta105_msonly_L1MU14FCH", "HLT_mu60_L1MU14FCH",
        "HLT_mu80_msonly_3layersEC_L1MU14FCH"
      };
    }

    bool trigPassed_SMT = false;
    if (mu){
      for(const auto& trig : single_mu_paths){
        bool pass = m_triggerdecos.at("trigPassed_"+trig).get(*event, sys);
        if (pass){
          bool match = m_matchingTool->match(*mu, trig);
          trigPassed_SMT |= match;
        }
      }
      double pt_threshold = m_pt_threshold[ZCC::SLT][ZCC::mu];
      trigPassed_SMT &= mu->pt() > pt_threshold;
    }

    m_bools.at(ZCC::pass_trigger_SLT) |= (trigPassed_SET || trigPassed_SMT);
  }


  void ZCharmSelectorAlg::evaluateLeptonCuts
  (const xAOD::ElectronContainer& electrons, const xAOD::MuonContainer& muons)
  {
    TLorentzVector ll;
    double mll = -99;

    bool OPPOSITE_CHARGE_LEPTONS = false;

    if (electrons.size() + muons.size() == 2) {
      m_bools.at(ZCC::EXACTLY_TWO_LEPTONS) = true;
    }

    if (electrons.size() >= 2)
    {
      m_bools.at(ZCC::IS_ee) = true;
      ll = electrons.at(0)->p4() + electrons.at(1)->p4();
      OPPOSITE_CHARGE_LEPTONS = electrons.at(0)->charge()*electrons.at(1)->charge() == -1;
    }

    if (muons.size() >= 2)
    {
      m_bools.at(ZCC::IS_mm) = true;
      ll = muons.at(0)->p4() + muons.at(1)->p4();
      OPPOSITE_CHARGE_LEPTONS = muons.at(0)->charge()*muons.at(1)->charge() == -1;
    }

    if (electrons.size() == 1 && muons.size() == 1)
    {
      m_bools.at(ZCC::IS_em) = true;
      ll = electrons.at(0)->p4() + muons.at(0)->p4();
      OPPOSITE_CHARGE_LEPTONS = electrons.at(0)->charge()*muons.at(0)->charge() == -1;
    }

    mll = ll.M();
    
    m_bools.at(ZCC::OPPOSITE_CHARGE_LEPTONS) = OPPOSITE_CHARGE_LEPTONS;
    m_bools.at(ZCC::DILEPTON_MASS_WINDOW) = ( mll >= 76.*Athena::Units::GeV && mll <= 106.*Athena::Units::GeV );
  }

  void ZCharmSelectorAlg::evaluateCJetCuts
  (const ConstDataVector<xAOD::JetContainer>& cjets)
  { 
    ///All jets in the containers should have pT>20GeV. Check minPt of your JetSelectorAlg in the ZCharm_config file.
    m_bools.at(ZCC::ONE_C_JETS) = (cjets.size() >= 1);
    m_bools.at(ZCC::TWO_C_JETS) = (cjets.size() >= 2);
  }  

  void ZCharmSelectorAlg::setThresholds(const xAOD::EventInfo* event,
					const CP::SystematicSet& sys) {

    int year = m_year.get(*event, sys);

    // Single-lepton triggers
    if(year==2015) {
      m_pt_threshold[ZCC::SLT][ZCC::ele] = 25. * Athena::Units::GeV;
    }
    // 2022 75 bunches
    else if(m_is22_75bunches.get(*event, sys)) {
      m_pt_threshold[ZCC::SLT][ZCC::ele] = 18. * Athena::Units::GeV;
    }
    else {
      m_pt_threshold[ZCC::SLT][ZCC::ele] = 27. * Athena::Units::GeV;
    }

    if(year==2015) {
      m_pt_threshold[ZCC::SLT][ZCC::mu] = 21. * Athena::Units::GeV;
    }
    else if(year>=2016 && year<=2018) {
      m_pt_threshold[ZCC::SLT][ZCC::mu] = 27. * Athena::Units::GeV;
    }
    else {
      m_pt_threshold[ZCC::SLT][ZCC::mu] = 25. * Athena::Units::GeV;
    }

  }

}
