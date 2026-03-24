/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

/// @author 

#include "BaselineVarsZbbjCalibAlg.h"

namespace XBBCALIB
{
  BaselineVarsZbbjCalibAlg::BaselineVarsZbbjCalibAlg(const std::string &name,
                                           ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  { }

  BaselineVarsZbbjCalibAlg::~BaselineVarsZbbjCalibAlg() = default;

  StatusCode BaselineVarsZbbjCalibAlg::initialize()
  {
    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    m_z_candidate_4vec = std::make_unique<FourVectorOutBlock>(
      this, "Zcand_", m_systematicsList, m_eventHandle);

    ATH_CHECK(m_lrjets_n.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsZbbjCalibAlg::execute()
  {
    //Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys));

      m_lrjets_n.set(*event, static_cast<int>(lrjets->size()), sys);

      if (lrjets->size() >= 1) {
        m_z_candidate_4vec->set(*event, *lrjets->at(0), sys);
      } else {
        m_z_candidate_4vec->setDefault(*event, sys);
      }
    }
    return StatusCode::SUCCESS;
  }

}
