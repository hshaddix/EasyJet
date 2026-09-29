/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


#include "PhSVJSelectorAlg.h"
#include "xAODEgamma/Electron.h"
#include "xAODEgamma/Photon.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthenaKernel/Units.h>
#include <algorithm>
#include <cmath>

namespace PhSVJ {

  PhSVJSelectorAlg::PhSVJSelectorAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator) {
  }

  StatusCode PhSVJSelectorAlg::initialize() {
    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    if (m_doLargeRJets) {
      ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    }
    if (m_doPhotons) {
        ATH_CHECK (m_photonHandle.initialize(m_systematicsList));
    }

    ATH_CHECK (m_metHandle.initialize(m_systematicsList));

    // Build trigger decoration handles from configured chain names
    m_triggerHandles.reserve(m_triggerChains.size());
    for (const std::string& chain : m_triggerChains) {
      std::string deco = chain;
      std::replace(deco.begin(), deco.end(), '-', '_');
      std::replace(deco.begin(), deco.end(), '.', 'p');
      deco = "trigPassed_" + deco;
      m_triggerHandles.emplace_back(deco, this);
      ATH_CHECK(m_triggerHandles.back().initialize(m_systematicsList, m_eventHandle));
    }

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }


  StatusCode PhSVJSelectorAlg::execute(const EventContext& ctx) {

    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector()) {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      if (m_doSmallRJets) {
          ANA_CHECK (m_jetHandle.retrieve (jets, sys, ctx));
      }

      const xAOD::JetContainer *lrjets = nullptr;
      if (m_doLargeRJets) {
        ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys, ctx));
      }

      const xAOD::PhotonContainer *photons = nullptr;
      if (m_doPhotons) {
          ANA_CHECK (m_photonHandle.retrieve (photons, sys, ctx));
      }

      
      const xAOD::MissingETContainer *metCont = nullptr;
      ANA_CHECK (m_metHandle.retrieve (metCont, sys, ctx));
      const xAOD::MissingET* met = (*metCont)["Final"];
      if (!met) {
      	ATH_MSG_ERROR("Could not retrieve MET");
      	return StatusCode::FAILURE;

      }
      
      bool passed_event = true;

      // Large-R jet multiplicity cut
      if (m_doLargeRJets && m_minLargeRJets >= 0) {
        if (!lrjets || static_cast<int>(lrjets->size()) < m_minLargeRJets)
          passed_event = false;
      }

      // Muon multiplicity cut
      if (m_doPhotons && m_minPhotons >= 0) {
        if (!photons || static_cast<int>(photons->size()) < m_minPhotons)
          passed_event = false;
      }

      // Trigger cut: event passes if any configured chain fired (OR logic)
      if(m_do_trigger_filtering) {
        if (passed_event && !m_triggerHandles.empty()) {
          bool passedTrigger = false;
          for (auto& handle : m_triggerHandles) {
            if (handle.get(*event, sys)) { passedTrigger = true; break; }
          }
          if (!passedTrigger) passed_event = false;
        }
      }

      if (!m_bypass && !passed_event) {
        continue;
      }
      filter.setPassed(true);
    }
    return StatusCode::SUCCESS;
  }

  StatusCode PhSVJSelectorAlg::finalize() {
    ANA_CHECK (m_filterParams.finalize());
    return StatusCode::SUCCESS;
  }
}
