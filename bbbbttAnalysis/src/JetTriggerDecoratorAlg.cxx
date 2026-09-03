/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


#include "JetTriggerDecoratorAlg.h"

#include "TriggerUtils.h"



namespace HHHBBBBTT
{

  StatusCode JetTriggerDecoratorAlg::initialize() {
    // Read syst-aware input handles
    ATH_CHECK(m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));

    ATH_CHECK(m_year.initialize(m_systematicsList, m_eventHandle));

    for(const auto& [runBool, name] : m_runBooleans) {
      SG::ReadDecorHandleKey<xAOD::EventInfo> deco;
      deco = "EventInfo." + name;
      m_runBooleans_key.emplace(runBool, deco);
      ATH_CHECK(m_runBooleans_key.at(runBool).initialize());
    }

    // make trigger decorators
    for (auto trig : m_triggers){
      SG::ReadDecorHandleKey<xAOD::EventInfo> deco;
      deco = "EventInfo.trigPassed_" + trig;
      m_triggerdecoKeys.emplace(trig, deco);
      ATH_CHECK(m_triggerdecoKeys.at(trig).initialize());

      CP::SysReadDecorHandle<float> decoHLTPt {"match_"+trig+"_HLTpt_%SYS%", this};
      m_HLTPTDecor.emplace(trig, decoHLTPt);
      ATH_CHECK(m_HLTPTDecor.at(trig).initialize(m_systematicsList, m_jetHandle));

      CP::SysReadDecorHandle<float> decoL1Et {"match_"+trig+"_L1et_%SYS%", this};
      m_L1ETDecor.emplace(trig, decoL1Et);
      ATH_CHECK(m_L1ETDecor.at(trig).initialize(m_systematicsList, m_jetHandle));

      CP::SysReadDecorHandle<float> decoL1Eta {"match_"+trig+"_L1eta_%SYS%", this};
      m_L1EtaDecor.emplace(trig, decoL1Eta);
      ATH_CHECK(m_L1EtaDecor.at(trig).initialize(m_systematicsList, m_jetHandle));
    }

    ATH_CHECK(m_jet_trigMatch_HLTPt.initialize(m_systematicsList, m_jetHandle));
    ATH_CHECK(m_jet_trigMatch_L1Et.initialize(m_systematicsList, m_jetHandle));
    ATH_CHECK(m_jet_trigMatch_L1Eta.initialize(m_systematicsList, m_jetHandle));

    ATH_CHECK (m_systematicsList.initialize());
    return StatusCode::SUCCESS;
  }


  StatusCode JetTriggerDecoratorAlg::execute() {
    trigReadDecoMap triggerdecos;
    for (const auto& [name, key] : m_triggerdecoKeys){
      triggerdecos.emplace(name, key);
    }
    runBoolReadDecoMap runBoolDecos;
    for (const auto& [runBool, key] : m_runBooleans_key){
      runBoolDecos.emplace(runBool, key);
    }
    
    for (const auto& sys : m_systematicsList.systematicsVector()) {
      const xAOD::EventInfo *event = nullptr;
      ATH_CHECK (m_eventHandle.retrieve (event, sys));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys));

      for (const xAOD::Jet *jet : *jets) {
	m_jet_trigMatch_L1Et.set(*jet, -99., sys);
	m_jet_trigMatch_L1Eta.set(*jet, -99., sys);
	m_jet_trigMatch_HLTPt.set(*jet, -99., sys);
      }

      checkDiTauTriggers(event, sys, runBoolDecos, triggerdecos, jets);
      checkDiBJetTriggers(event, sys, runBoolDecos, triggerdecos, jets);

    }

    return StatusCode::SUCCESS;
  }

  void JetTriggerDecoratorAlg::checkDiTauTriggers
  (const xAOD::EventInfo* eventInfo, const CP::SystematicSet& sys,
   const runBoolReadDecoMap& runBoolDecos, const trigReadDecoMap& triggerdecos,
   const xAOD::JetContainer* jets) const {

    std::vector<std::string> ditau_paths_2016;
    std::vector<std::string> ditau_paths_L1Topo;
    std::vector<std::string> ditau_paths_4J12;
    std::vector<std::string> ditau_paths_L1Topo_delayed;
    std::vector<std::string> ditau_paths_4J12_delayed;
    std::vector<std::string> tau35_match_paths;
    std::vector<std::string> tau25_match_paths;

    getDiTauTriggers(m_year.get(*eventInfo, sys), eventInfo, runBoolDecos,
		     ditau_paths_2016, ditau_paths_L1Topo, ditau_paths_4J12,
		     ditau_paths_L1Topo_delayed, ditau_paths_4J12_delayed,
		     tau35_match_paths, tau25_match_paths);

    std::unordered_map<HHHBBBBTT::TriggerChannel, std::vector<std::string>> mapPaths;
    mapPaths.emplace(HHHBBBBTT::DTT_4J12, ditau_paths_4J12);
    mapPaths.emplace(HHHBBBBTT::DTT_L1Topo, ditau_paths_L1Topo);
    mapPaths.emplace(HHHBBBBTT::DTT_4J12_delayed, ditau_paths_4J12_delayed);
    mapPaths.emplace(HHHBBBBTT::DTT_L1Topo_delayed, ditau_paths_L1Topo_delayed);

    for(const auto& [channel, paths] : mapPaths){
      for(const auto& trig : paths){
	bool pass = triggerdecos.at(trig)(*eventInfo);
	if(pass){
	  for (const xAOD::Jet *jet : *jets){
	    m_jet_trigMatch_L1Et.set(*jet, m_L1ETDecor.at(trig).get(*jet, sys), sys);
	    m_jet_trigMatch_L1Eta.set(*jet, m_L1EtaDecor.at(trig).get(*jet, sys), sys);
	  }
	}
      }
    }

  }

  void JetTriggerDecoratorAlg::checkDiBJetTriggers
  (const xAOD::EventInfo* eventInfo, const CP::SystematicSet& sys,
   const runBoolReadDecoMap& runBoolDecos, const trigReadDecoMap& triggerdecos,
   const xAOD::JetContainer* jets) const {

    std::vector<std::string> dib_paths;
    getDiBJetTriggers(m_year.get(*eventInfo, sys), eventInfo, runBoolDecos, dib_paths);

    std::unordered_map<HHHBBBBTT::TriggerChannel, std::vector<std::string>> mapPaths;
    mapPaths.emplace(HHHBBBBTT::DBT, dib_paths);

    for(const auto& [channel, paths] : mapPaths){
      for(const auto& trig : paths){
	bool pass = triggerdecos.at(trig)(*eventInfo);
	if(pass){
	  for (const xAOD::Jet *jet : *jets){
	    m_jet_trigMatch_L1Et.set(*jet, m_L1ETDecor.at(trig).get(*jet, sys), sys);
	    m_jet_trigMatch_L1Eta.set(*jet, m_L1EtaDecor.at(trig).get(*jet, sys), sys);
	    m_jet_trigMatch_HLTPt.set(*jet, m_HLTPTDecor.at(trig).get(*jet, sys), sys);
	  }
	}
      }
    }

  }

}
