/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZllyCalibSelectorAlg.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthenaKernel/Units.h>

namespace XBBCALIB
{
  StatusCode ZllyCalibSelectorAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     ZllyCalibSelectorAlg      \n");
    ATH_MSG_INFO("*********************************\n");
    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));

    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    if (m_saveCutFlow) ATH_CHECK (m_generatorWeight.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_photonHandle.initialize(m_systematicsList));

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


  StatusCode ZllyCalibSelectorAlg::execute(const EventContext& ctx)
  {

    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Initialize Variables
      bool pass_baseline = false;
      for (auto& [key, value] : m_boolnames) m_bools.at(key) = false;

      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ATH_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *lrjets = nullptr;
      ATH_CHECK (m_lrjetHandle.retrieve (lrjets, sys, ctx));

      const xAOD::MuonContainer *muons = nullptr;
      ATH_CHECK (m_muonHandle.retrieve (muons, sys, ctx));
      
      const xAOD::ElectronContainer *electrons = nullptr;
      ATH_CHECK (m_electronHandle.retrieve (electrons, sys, ctx));

      const xAOD::PhotonContainer *photons = nullptr;
      ATH_CHECK (m_photonHandle.retrieve(photons, sys, ctx));

      //************
      // Apply Selection
      //************
      if (electrons->size() >= 2 || muons->size() >= 2){
        m_bools.at(XBBCALIB::PASS_TWO_SF_LEPTONS) = true;}
      if (photons->size() == 1){
        m_bools.at(XBBCALIB::PASS_EXACTLY_ONE_PHOTON) = true;}

      pass_baseline = m_bools.at(XBBCALIB::PASS_TWO_SF_LEPTONS) && m_bools.at(XBBCALIB::PASS_EXACTLY_ONE_PHOTON);

      m_passallcuts.set(*event, pass_baseline, sys);

      if (sys.name()=="" && m_saveCutFlow){
	std::vector<bool> cutBools;
	for(const auto& cut : m_inputCutList)
	  cutBools.push_back(m_bools.at(m_nameToBool.at(cut)));
	double mc_weight = m_isMC ? m_generatorWeight.get(*event, sys) : 0.;
	ATH_CHECK (applyCutFlow(cutBools, mc_weight));
      }

      //****************
      // event level info
      //****************
      // Fill syst-aware output decorators
      for (auto& [key, var] : m_bools) {
        m_Bbranches.at(key).set(*event, var, sys);
      }
      
      if (!m_bypass && !pass_baseline) continue;

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZllyCalibSelectorAlg::finalize()
  {
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);
    ATH_CHECK (m_filterParams.finalize());
    ATH_CHECK (finaliseCutflow());
    return StatusCode::SUCCESS;
  }
}
