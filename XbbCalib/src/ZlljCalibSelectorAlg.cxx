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

    // special flag for all cuts
    ATH_CHECK (m_passallcuts.initialize(m_systematicsList, m_eventHandle));

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
      pass_baseline = (lrjets->size() == 1);

      bool pass_ele = false;
      bool pass_mu = false;
      int n_ele = electrons->size();
      int n_muon = muons->size();
      int n_lept = n_muon + n_ele;

      //************
      // Z candidate kinematics
      //************
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

      // Additonal kinematic cuts
      if (pass_baseline) {
        // add mass and pt requirements
        pass_baseline &= (ll.Pt() > 200000. && ll.Pt() < 3000000.);
        pass_baseline &= (ll.M() > 70000. && ll.M() < 110000.);
        
        const xAOD::Jet *j0 = lrjets->at(0);
        // Delta Phi separation (j,Z)
        if (m_deltaPhiCut > 0) {    
          float deltaPhi_ll_jet = j0->p4().DeltaPhi(ll);
          pass_baseline &= std::abs(deltaPhi_ll_jet) > m_deltaPhiCut;
        }
        // pT asymmetry using BJR pT: (pt_ll - pt_j0) / (pt_ll + pt_j0) < cut
        if (m_asymmetryCut > 0) {
          float ptSum = j0->pt() + ll.Pt();
          bool pass_asymmetry = (ptSum > 0) && ((ll.Pt() - j0->pt()) / ptSum < m_asymmetryCut);
          pass_baseline &= (ll.Pt() > j0->pt());
          pass_baseline &= pass_asymmetry;
        }
        // Rapidity difference: |y0 - y1| < cut
        if (m_rapidityCut > 0) {
          bool pass_rapidity = std::abs(j0->rapidity() - ll.Rapidity()) < m_rapidityCut;
          pass_baseline &= pass_rapidity;
        }
      }
      m_passallcuts.set(*event, pass_baseline, sys);


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

