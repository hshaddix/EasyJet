/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZllyCalibSelectorAlg.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthenaKernel/Units.h>
#include "TLorentzVector.h"

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

      //************
      // Photons
      //************
      pass_baseline = (photons->size() == 1);

      bool pass_ele = false;
      bool pass_mu = false;
      int n_ele = electrons->size();
      int n_muon = muons->size();
      int n_lept = n_muon + n_ele;

      //************
      // Z(+photon) system kinematics
      //************
      
      // Dilepton (Z candidate) four-vector from the two leading leptons
      TLorentzVector ll;
      if (n_ele == 2) {
        ll = electrons->at(0)->p4() + electrons->at(1)->p4();
        pass_ele = true;
      } else if (n_muon == 2) {
        ll = muons->at(0)->p4() + muons->at(1)->p4();
        pass_mu = true;
      }
      // veto possible states with more than 2 leptons 
      pass_baseline &= (pass_ele != pass_mu);
      if (n_lept != 2) pass_baseline = false;

      // add delta_phi separation, mass and pt requirements
      if (pass_baseline) {
        const xAOD::Photon * ph0 = photons->at(0);
        m_bools.at(XBBCALIB::PASS_TWO_SF_LEPTONS) = (pass_ele || pass_mu);
        m_bools.at(XBBCALIB::PASS_EXACTLY_ONE_PHOTON) = (photons->size() == 1);
        m_bools.at(XBBCALIB::PASS_ZPT_CUT)   = (ll.Pt() > 200000. && ll.Pt() < 500000.);
        m_bools.at(XBBCALIB::PASS_ZMASS_WINDOW) = (ll.M()  > 70000.  && ll.M()  < 110000.);

        // TODO: is this pt cut needed? 
        float deltaPhi_ll_ph = ph0->p4().DeltaPhi(ll);
        m_bools.at(XBBCALIB::PASS_DPHI_ZGAMMA)  = std::abs(deltaPhi_ll_ph) > 1.5707963;

        pass_baseline &= m_bools.at(XBBCALIB::PASS_ZPT_CUT);
        pass_baseline &= m_bools.at(XBBCALIB::PASS_ZMASS_WINDOW);
        pass_baseline &= m_bools.at(XBBCALIB::PASS_DPHI_ZGAMMA);
      }

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




