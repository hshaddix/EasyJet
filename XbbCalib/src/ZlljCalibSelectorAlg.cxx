/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZlljCalibSelectorAlg.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthenaKernel/Units.h>
#include "TLorentzVector.h"

namespace XBBCALIB
{
  StatusCode ZlljCalibSelectorAlg::initialize()
  {
    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));

    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_muonHandle.initialize(m_systematicsList));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize()); 

    return StatusCode::SUCCESS;
  }


  StatusCode ZlljCalibSelectorAlg::execute(const EventContext& ctx)
  {

    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Initialize Variables
      bool pass_baseline = false;

      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys, ctx));

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK (m_muonHandle.retrieve (muons, sys, ctx));
      
      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK (m_electronHandle.retrieve (electrons, sys, ctx));

      //************
      // Apply Selection
      //************

      //************
      // Large-R Jet
      //************
      pass_baseline = (lrjets->size() != 0);
      const xAOD::Jet* j0 = nullptr;
      if (pass_baseline) {
        j0 = lrjets->at(0);
        pass_baseline = (j0->pt() > 200000.) && (std::abs(j0->eta()) < 2.0);
      }

      bool pass_ele = (electrons->size() == 2);
      bool pass_mu = (muons->size() == 2);
      pass_baseline &= (pass_ele || pass_mu);
      //************
      // Z candidate kinematics
      //************
      if (pass_baseline) {
        TLorentzVector ll;
        if (pass_ele) {
          ll = electrons->at(0)->p4() + electrons->at(1)->p4();
        } else {
          ll = muons->at(0)->p4() + muons->at(1)->p4();
        }
        pass_baseline &= (ll.Pt() > 500000. && ll.Pt() < 1000000.);
        pass_baseline &= (ll.M() > 70000. && ll.M() < 110000.);
        pass_baseline &= (std::abs(ll.DeltaPhi(j0->p4())) > 1.5);
      }


      if (!m_bypass && !pass_baseline) continue;

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZlljCalibSelectorAlg::finalize()
  {
    ANA_CHECK (m_filterParams.finalize());
    return StatusCode::SUCCESS;
  }

}

