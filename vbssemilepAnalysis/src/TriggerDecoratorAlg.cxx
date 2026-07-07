/*
   Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration   
*/

#include "TriggerDecoratorAlg.h"
#include "TriggerUtils.h"
#include <AthenaKernel/Units.h>

namespace VBSSEMILEP
{
  StatusCode TriggerDecoratorAlg::initialize()
  {
    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_year.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_jetsHandle.initialize(m_systematicsList));

    ////////////////// Trigger ///////////////////
    
    // make trigger decorators
    for (auto trig : m_triggers){
      CP::SysReadDecorHandle<bool> deco {trig, this};
      m_triggerdecos.emplace(trig, deco);
      ATH_CHECK(m_triggerdecos.at(trig).initialize(m_systematicsList, m_eventHandle));
    }

    // make trigger flags
    for (const auto& [channel, name] : m_triggerChannels) {
      m_trig_bools.emplace(channel, false);
      CP::SysWriteDecorHandle<char> whandle{"pass_trigger_"+name+"_%SYS%", this};
      m_trig_branches.emplace(channel, whandle);
      ATH_CHECK(m_trig_branches.at(channel).initialize(m_systematicsList, m_eventHandle));
    }

    // make bool for trig paths
    for(const auto& [runBool, name] : m_runBooleans) {
      SG::ReadDecorHandleKey<xAOD::EventInfo> deco;
      deco = "EventInfo." + name;
      m_runBooleans_key.emplace(runBool, deco);
      ATH_CHECK(m_runBooleans_key.at(runBool).initialize());
    }

    if(m_saveHighLevelVariables) {
      ANA_CHECK(m_ele0_passSET_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_ele1_passSET_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_mu0_passSMT_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_mu1_passSMT_decor.initialize(m_systematicsList, m_eventHandle));

      ANA_CHECK(m_ele0_trigPassed_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_ele1_trigPassed_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_mu0_trigPassed_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_mu1_trigPassed_decor.initialize(m_systematicsList, m_eventHandle));

      ANA_CHECK(m_ele0_trigMatched_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_ele1_trigMatched_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_mu0_trigMatched_decor.initialize(m_systematicsList, m_eventHandle));
      ANA_CHECK(m_mu1_trigMatched_decor.initialize(m_systematicsList, m_eventHandle));
    }

    ////////////////////////////////

    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode TriggerDecoratorAlg::execute(const EventContext& ctx)
  {
    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector()){
      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK (m_muonHandle.retrieve (muons, sys, ctx));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK (m_electronHandle.retrieve (electrons, sys, ctx));
      
      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetsHandle.retrieve (jets, sys, ctx));

      runBoolReadDecoMap runBoolDecos;
      for (const auto& [runBool, key] : m_runBooleans_key){
        runBoolDecos.emplace(runBool, key);
      }

      for (const auto& [channel, name] : m_triggerChannels) m_trig_bools.at(channel) = false;

      setThresholds(event, runBoolDecos, sys);
      evaluateTriggerCuts(event, runBoolDecos, electrons, muons, sys);
      

      // check if trigger is fired or not
      checkSingleJetTrigger(event, sys);

      for (auto& [channel, var] : m_trig_bools) {
        m_trig_branches.at(channel).set(*event, var, sys);
      }

      // check trigger matching
      //for(auto jet : *jets )
      //  checkSingleJetTriggerMatching(event, jet, sys);

    }
    return StatusCode::SUCCESS;
  }

  void TriggerDecoratorAlg::checkSingleJetTrigger
  (const xAOD::EventInfo *event, const CP::SystematicSet& sys)
  {

    // Check single jets triggers
    int year = m_year.get(*event, sys);
    std::vector<std::string> trigger_list = getSingleLargeRJetsTriggers(year);
    
    bool trigPassed_SJT = false;
    for(const auto& trig : trigger_list){
      bool pass = m_triggerdecos.at("trigPassed_" + trig).get(*event, sys);
      trigPassed_SJT |= pass;
    }

    if(trigPassed_SJT){
      m_trig_bools.at(VBSSEMILEP::SJT) = true;
    }

    return;
  }

  void TriggerDecoratorAlg::checkSingleJetTriggerMatching
  (const xAOD::EventInfo *event, const xAOD::Jet *jet, const CP::SystematicSet& sys)
  {

    // Check single jets triggers
    int year = m_year.get(*event, sys);
    std::vector<std::string> trigger_list = getSingleLargeRJetsTriggers(year);
    
    bool trigPassed_SJT = false;
    if(jet){
      for(const auto& trig : trigger_list){
        bool pass = m_triggerdecos.at("trigPassed_" + trig).get(*event, sys);
        if (pass){
          bool match = m_matchingTool->match(*jet, trig);
          trigPassed_SJT |= match;
        }
      }
    }

    if(trigPassed_SJT){
      m_trig_bools.at(VBSSEMILEP::SJTM) = true;
    }

    return;
  }
  
  void TriggerDecoratorAlg::evaluateTriggerCuts(const xAOD::EventInfo *event, const runBoolReadDecoMap& runBoolDecos,
                                                const xAOD::ElectronContainer *electrons , const xAOD::MuonContainer *muons, const CP::SystematicSet& sys) {

    //Leptons
    const xAOD::Electron* ele0 = nullptr;
    const xAOD::Electron* ele1 = nullptr;
    
    const xAOD::Muon* mu0 = nullptr;
    const xAOD::Muon* mu1 = nullptr;

    if (electrons->size() >= 1) ele0 = electrons->at(0);
    if (electrons->size() >= 2) ele1 = electrons->at(1);
    if (muons->size() >= 1) mu0 = muons->at(0);
    if (muons->size() >= 2) mu1 = muons->at(1);

    bool ele0_passSET = false;
    bool ele1_passSET = false;
    bool mu0_passSMT = false;
    bool mu1_passSMT = false;

    std::vector<std::string> ele0_trigPassed;
    std::vector<std::string> ele1_trigPassed;
    std::vector<std::string> mu0_trigPassed;
    std::vector<std::string> mu1_trigPassed;

    std::vector<std::string> ele0_trigMatched;
    std::vector<std::string> ele1_trigMatched;
    std::vector<std::string> mu0_trigMatched;
    std::vector<std::string> mu1_trigMatched;

    
    // Based on decision of group, only SLT will remain in the analysis code
    if (ele0 || mu0)
      evaluateSingleLeptonTrigger(event, runBoolDecos, ele0, mu0, sys,
				  ele0_trigPassed, mu0_trigPassed,
				  ele0_trigMatched, mu0_trigMatched,
				  ele0_passSET, mu0_passSMT);
    if (ele1 || mu1)
      evaluateSingleLeptonTrigger(event, runBoolDecos, ele1, mu1, sys,
				  ele1_trigPassed, mu1_trigPassed,
				  ele1_trigMatched, mu1_trigMatched,
				  ele1_passSET, mu1_passSMT);

    if(m_saveHighLevelVariables) {
      // Store the output as decorators which will be written into the nTuples
      m_ele0_passSET_decor.set(*event, ele0_passSET, sys);
      m_ele1_passSET_decor.set(*event, ele1_passSET, sys);
      m_mu0_passSMT_decor.set(*event, mu0_passSMT, sys);
      m_mu1_passSMT_decor.set(*event, mu1_passSMT, sys);

      m_ele0_trigPassed_decor.set(*event, ele0_trigPassed, sys);
      m_ele1_trigPassed_decor.set(*event, ele1_trigPassed, sys);
      m_mu0_trigPassed_decor.set(*event, mu0_trigPassed, sys);
      m_mu1_trigPassed_decor.set(*event, mu1_trigPassed, sys);

      m_ele0_trigMatched_decor.set(*event, ele0_trigMatched, sys);
      m_ele1_trigMatched_decor.set(*event, ele1_trigMatched, sys);
      m_mu0_trigMatched_decor.set(*event, mu0_trigMatched, sys);
      m_mu1_trigMatched_decor.set(*event, mu1_trigMatched, sys);

    }
  }

  void TriggerDecoratorAlg::evaluateSingleLeptonTrigger
  (const xAOD::EventInfo *event, const runBoolReadDecoMap& runBoolDecos,
   const xAOD::Electron *ele, const xAOD::Muon *mu,
   const CP::SystematicSet& sys,
   std::vector<std::string>& ele_trigPassed, std::vector<std::string>& mu_trigPassed,
   std::vector<std::string>& ele_trigMatched, std::vector<std::string>& mu_trigMatched,
   bool& ele_passSET, bool& mu_passSMT)
  {
    // Check single electron triggers
    std::vector<std::string> single_ele_paths;
    int year = m_year.get(*event, sys);
    getSingleEleTriggers(year, event, runBoolDecos, single_ele_paths);

    bool trigPassed_SET = false;
    if(ele){
      for(const auto& trig : single_ele_paths){
        bool pass = m_triggerdecos.at("trigPassed_"+trig).get(*event, sys);
        if (pass){
          if(m_saveHighLevelVariables) ele_trigPassed.push_back(trig); // Record trigger name if it is passed for the electron
          bool match = m_matchingTool->match(*ele, trig);
          if (match && m_saveHighLevelVariables) ele_trigMatched.push_back(trig); // Record trigger name if it is matched for the electron
          trigPassed_SET |= match;
        }
      }
      trigPassed_SET &= ele->pt() > m_pt_threshold[VBSSEMILEP::SLT][VBSSEMILEP::ele];
      ele_passSET = trigPassed_SET;
    }

    // Check single muon triggers
    std::vector<std::string> single_mu_paths;
    getSingleMuTriggers(year, event, runBoolDecos, single_mu_paths);

    bool trigPassed_SMT = false;
    if (mu){
      for(const auto& trig : single_mu_paths){
        bool pass = m_triggerdecos.at("trigPassed_"+trig).get(*event, sys);
        if (pass){
          if(m_saveHighLevelVariables) mu_trigPassed.push_back(trig); // Record trigger name if it is passed for the muon
          bool match = m_matchingTool->match(*mu, trig);
          if (match && m_saveHighLevelVariables) mu_trigMatched.push_back(trig); // Record trigger name if it is matched for the muon
          trigPassed_SMT |= match;
        }
      }
      trigPassed_SMT &= mu->pt() > m_pt_threshold[VBSSEMILEP::SLT][VBSSEMILEP::mu];
      mu_passSMT = trigPassed_SMT;

    }

    if(trigPassed_SET || trigPassed_SMT){
      m_trig_bools.at(VBSSEMILEP::SLT) = true;
    }
  }

  void TriggerDecoratorAlg::setThresholds(const xAOD::EventInfo* event,
					const runBoolReadDecoMap& runBoolDecos, const CP::SystematicSet& sys) {
    
    int year = m_year.get(*event, sys);

    // Single-lepton triggers
    if(year==2015)
      m_pt_threshold[VBSSEMILEP::SLT][VBSSEMILEP::ele] = 25. * Athena::Units::GeV;
    // 2022 75 bunches
    else if(runBoolDecos.at(VBSSEMILEP::is22_75bunches)(*event))
      m_pt_threshold[VBSSEMILEP::SLT][VBSSEMILEP::ele] = 18. * Athena::Units::GeV;
    else
      m_pt_threshold[VBSSEMILEP::SLT][VBSSEMILEP::ele] = 27. * Athena::Units::GeV;

    if(year==2015)
      m_pt_threshold[VBSSEMILEP::SLT][VBSSEMILEP::mu] = 21. * Athena::Units::GeV;
    else if(year>=2016 && year<=2018)
      m_pt_threshold[VBSSEMILEP::SLT][VBSSEMILEP::mu] = 27. * Athena::Units::GeV;
    else
      m_pt_threshold[VBSSEMILEP::SLT][VBSSEMILEP::mu] = 25. * Athena::Units::GeV;
  }
}
