/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZtautauCalibSelectorAlg.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthenaKernel/Units.h>

namespace XBBCALIB
{

  ZtautauCalibSelectorAlg::ZtautauCalibSelectorAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {

  }

  StatusCode ZtautauCalibSelectorAlg::initialize()
  {
    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    ATH_CHECK (m_lRjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize()); 

    return StatusCode::SUCCESS;
  }


  StatusCode ZtautauCalibSelectorAlg::execute(const EventContext& ctx)
  {

    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Initialize Variables
      bool pass_baseline = false;

      // Retrive inputs
      const xAOD::JetContainer *lRjets = nullptr;
      ANA_CHECK (m_lRjetHandle.retrieve (lRjets, sys, ctx));
      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK(m_electronHandle.retrieve (electrons, sys, ctx));


      //************
      // Large-R Jet
      //************
      int n_boosted_jets = lRjets->size();

      //************
      // electrons
      //************
      int n_electrons = electrons->size();
      
      //************
      // Apply Selection
      //************
      float deltaPhi_largeR = 0;
      if (n_boosted_jets >= 2) {
        const xAOD::Jet *j0 = lRjets->at(0);
        const xAOD::Jet *j1 = lRjets->at(1);
        deltaPhi_largeR = j0->p4().DeltaPhi(j1->p4());
      }

      // Need to have a at least 2 large-R jets with DPhi > 1.5 and no electrons
      pass_baseline = n_boosted_jets == 2 and n_electrons == 0 and abs(deltaPhi_largeR) >= 1.5;

      if (!m_bypass && !pass_baseline) continue;

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZtautauCalibSelectorAlg::finalize()
  {
    ANA_CHECK (m_filterParams.finalize());
    return StatusCode::SUCCESS;
  }

}
