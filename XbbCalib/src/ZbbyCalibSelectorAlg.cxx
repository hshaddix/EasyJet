/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#include "ZbbyCalibSelectorAlg.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthenaKernel/Units.h>

namespace XBBCALIB
{

  ZbbyCalibSelectorAlg::ZbbyCalibSelectorAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {

  }

  StatusCode ZbbyCalibSelectorAlg::initialize()
  {
    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));

    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_photonHandle.initialize(m_systematicsList));


    if (m_remove_zcc_overlap) m_isCC_acc.emplace("z_is_cc");

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }


  StatusCode ZbbyCalibSelectorAlg::execute()
  {

    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys));

      // Veto Z->cc events from Zqq samples (Zcc/Zqq overlap removal)
      if (m_isCC_acc && (*m_isCC_acc)(*event) == 1) continue;

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys));

      const xAOD::PhotonContainer *photons = nullptr;
      ANA_CHECK (m_photonHandle.retrieve(photons, sys));
      if (lrjets->empty()) continue;

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZbbyCalibSelectorAlg::finalize()
  {
    ANA_CHECK (m_filterParams.finalize());
    return StatusCode::SUCCESS;
  }

}

