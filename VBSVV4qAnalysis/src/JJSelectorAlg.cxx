/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JJSelectorAlg.h"
#include <AthenaKernel/Units.h>

#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>

namespace VBSVV4q{
  StatusCode JJSelectorAlg::initialize(){

    ATH_MSG_INFO("*********************\n");
    ATH_MSG_INFO("     JJSelectorAlg   \n");
    ATH_MSG_INFO("*********************\n");

    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    ATH_CHECK (m_passTriggerSJT.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK (m_vbsjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_LargeRJetsHandle.initialize(m_systematicsList));
    ATH_CHECK (m_GN2Xv01_phbb.initialize(m_systematicsList, m_LargeRJetsHandle));
    ATH_CHECK (m_GN2Xv01_phcc.initialize(m_systematicsList, m_LargeRJetsHandle));
    ATH_CHECK (m_GN2Xv01_pqcd.initialize(m_systematicsList, m_LargeRJetsHandle));
    ATH_CHECK (m_GN2Xv01_ptop.initialize(m_systematicsList, m_LargeRJetsHandle));

    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    if (m_saveCutFlow) ATH_CHECK (m_generatorWeight.initialize(m_systematicsList, m_eventHandle));

    for (auto& [key, value] : m_boolnames) {
      m_bools.emplace(key, false);
      CP::SysWriteDecorHandle<char> whandle{value+"_%SYS%", this};
      m_Bbranches.emplace(key, whandle);
      ATH_CHECK(m_Bbranches.at(key).initialize(m_systematicsList, m_eventHandle));
    }

    // special flag for all cuts
    ATH_CHECK (m_passallcuts.initialize(m_systematicsList, m_eventHandle));

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
  
  StatusCode JJSelectorAlg::execute(const EventContext& ctx){
    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector()){
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ATH_CHECK (m_eventHandle.retrieve (event, sys, ctx));
  
      const xAOD::JetContainer *vbsjets = nullptr;
      if( !m_UseVBFRNN )
        ATH_CHECK (m_vbsjetHandle.retrieve (vbsjets, sys, ctx));

      const xAOD::JetContainer *largeJets = nullptr;
      ATH_CHECK (m_LargeRJetsHandle.retrieve (largeJets, sys, ctx));

      for (auto& [key, value] : m_boolnames) m_bools.at(key) = false;

      if ( largeJets->size() >= 1 )  m_bools.at(VBSVV4q::PASS_ONE_LARGE_JET) = true;
      
      if (!m_passTriggerSJT.empty() and m_passTriggerSJT.get(*event, sys)) {
        m_bools.at(VBSVV4q::PASS_TRIGGER) = true;
      } 

      if( !m_UseVBFRNN )
        vbsjetsSelection(vbsjets);

      bool pass_preselection = true;
      //bool pass_preselection = m_bools.at(VBSVV4q::PASS_RES_BASELINE) || m_bools.at(VBSVV4q::PASS_MERG_BASELINE);
      m_passallcuts.set(*event, pass_preselection, sys);

      if (sys.name()=="" && m_saveCutFlow){
	std::vector<bool> cutBools;
	for(const auto& cut : m_inputCutList)
	  cutBools.push_back(m_bools.at(m_nameToBool.at(cut)));
	double mc_weight = m_isMC ? m_generatorWeight.get(*event, sys) : 0.;
	ATH_CHECK (applyCutFlow(cutBools, mc_weight));
      }

      // Fill syst-aware output decorators
      for (auto& [key, var] : m_bools) {
        m_Bbranches.at(key).set(*event, var, sys);
      }

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      if (!m_bypass && !pass_preselection) continue;
      filter.setPassed(true);
    }
    return StatusCode::SUCCESS;
  }

  StatusCode JJSelectorAlg::finalize(){

    //adapt the following for each syst TODO
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);
    ATH_CHECK (m_filterParams.finalize ());
    ATH_CHECK (finaliseCutflow());

    return StatusCode::SUCCESS;
  }

  void JJSelectorAlg :: vbsjetsSelection(const xAOD::JetContainer * vbsjets){
    
    if (vbsjets->size() >= 2){
      //TODO, create an object holding the vbs jets passing the mjj + deta selections
      const xAOD::Jet* vbsJet1 = vbsjets->at(0);
      const xAOD::Jet* vbsJet2 = vbsjets->at(1);;
      double mjj = (vbsJet1->p4() + vbsJet2->p4()).M();
      double dEta_jj = std::abs(vbsJet1->eta() - vbsJet2->eta());

      if ( mjj > 300 * Athena::Units::GeV  && dEta_jj > 3.0 ) {
        //m_bools.at(VBSVV4q::PASS_VBS_BASELINE) = true;
      }
    }

  }//vbsjetsSelection

}//name space
