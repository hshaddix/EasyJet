/*
   Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "HadTriggerDecoratorAlg.h"
#include "TriggerUtils.h"
#include <AthenaKernel/Units.h>

namespace VBSHIGGS
{
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

    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode HadTriggerDecoratorAlg::execute(const EventContext& ctx)
  {
    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector()){
      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

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

  void HadTriggerDecoratorAlg::evaluateTriggerCuts(const xAOD::EventInfo *event,
						   const runBoolReadDecoMap& runBoolDecos,
						   const CP::SystematicSet& sys) {

    std::vector<std::string> single_small_jet_paths;
    std::vector<std::string> single_large_jet_paths;

    const int year = m_year.get(*event, sys);
    getSingleSmallJetTriggers(year, event, runBoolDecos, single_small_jet_paths);
    getSingleLargeJetTriggers(year, event, runBoolDecos, single_large_jet_paths);

    bool trigPassed_smallJet = false;
    bool trigPassed_largeJet = false;
    evaluateSingleJetTrigger(event, single_small_jet_paths, trigPassed_smallJet, sys);
    evaluateSingleJetTrigger(event, single_large_jet_paths, trigPassed_largeJet, sys);

    m_trig_bools.at(VBSHIGGS::JET) = trigPassed_smallJet || trigPassed_largeJet;

  }

  void HadTriggerDecoratorAlg::evaluateSingleJetTrigger(const xAOD::EventInfo* event,
							const std::vector<std::string>& jet_paths,
							bool& jet_pass,
							const CP::SystematicSet& sys) {
    for (const auto& trig : jet_paths) {
      std::string trig_key = "trigPassed_" + trig;

      bool pass = m_triggerdecos.at(trig_key).get(*event, sys);
      if (pass) {
        jet_pass = true;
      }
    }
  }


  void HadTriggerDecoratorAlg::setThresholds(const xAOD::EventInfo* event,
					const runBoolReadDecoMap& runBoolDecos, const CP::SystematicSet& sys) {

    (void)runBoolDecos;
    (void)m_year.get(*event, sys);
  }
}
