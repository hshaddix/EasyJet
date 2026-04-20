/*
   Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "HadTriggerDecoratorAlg.h"
#include "TriggerUtils.h"
#include <AthenaKernel/Units.h>

namespace VBSHIGGS
{
  HadTriggerDecoratorAlg::HadTriggerDecoratorAlg(const std::string &name,
					   ISvcLocator *pSvcLocator)
      : EL::AnaAlgorithm(name, pSvcLocator){}

  StatusCode HadTriggerDecoratorAlg::initialize()
  {
    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_year.initialize(m_systematicsList, m_eventHandle));

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
    //   ANA_CHECK(m_ele0_passSET_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_ele1_passSET_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_mu0_passSMT_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_mu1_passSMT_decor.initialize(m_systematicsList, m_eventHandle));

    //   ANA_CHECK(m_ele0_trigPassed_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_ele1_trigPassed_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_mu0_trigPassed_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_mu1_trigPassed_decor.initialize(m_systematicsList, m_eventHandle));

    //   ANA_CHECK(m_ele0_trigMatched_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_ele1_trigMatched_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_mu0_trigMatched_decor.initialize(m_systematicsList, m_eventHandle));
    //   ANA_CHECK(m_mu1_trigMatched_decor.initialize(m_systematicsList, m_eventHandle));
    }

    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode HadTriggerDecoratorAlg::execute()
  {
    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector()){
      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys));

      runBoolReadDecoMap runBoolDecos;
      for (const auto& [runBool, key] : m_runBooleans_key){
        runBoolDecos.emplace(runBool, key);
      }

      for (const auto& [channel, name] : m_triggerChannels) m_trig_bools.at(channel) = false;

      setThresholds(event, runBoolDecos, sys);
      evaluateTriggerCuts(event, runBoolDecos, sys);

      for (auto& [channel, var] : m_trig_bools) {
        m_trig_branches.at(channel).set(*event, var, sys);
      }

    }
    return StatusCode::SUCCESS;
  }

  void HadTriggerDecoratorAlg::evaluateTriggerCuts(const xAOD::EventInfo *event, const runBoolReadDecoMap& runBoolDecos, const CP::SystematicSet& sys) {

    (void)runBoolDecos;

    bool trigPassed_HAD = false;
    for (const auto& trig : m_triggers) {
      auto it = m_triggerdecos.find(trig);
      if (it == m_triggerdecos.end()) {
        ATH_MSG_WARNING("Missing trigger decorator for " << trig);
        continue;
      }
      trigPassed_HAD |= it->second.get(*event, sys);
    }

    m_trig_bools.at(VBSHIGGS::HAD) = trigPassed_HAD;

  }


  void HadTriggerDecoratorAlg::setThresholds(const xAOD::EventInfo* event,
					const runBoolReadDecoMap& runBoolDecos, const CP::SystematicSet& sys) {

    (void)runBoolDecos;
    (void)m_year.get(*event, sys);
  }
}
