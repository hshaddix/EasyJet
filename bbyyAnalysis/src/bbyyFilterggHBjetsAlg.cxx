/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "bbyyFilterggHBjetsAlg.h"
#include <EventBookkeeperTools/FilterReporter.h>

namespace HHBBYY
{

  bbyyFilterggHBjetsAlg::bbyyFilterggHBjetsAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }


  StatusCode bbyyFilterggHBjetsAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("      bbyyFilterggHBjetsAlg      \n");
    ATH_MSG_INFO("*********************************\n");

    if (!m_truthBottomInKey.empty())
      ATH_CHECK(m_truthBottomInKey.initialize());
 
    ATH_CHECK (m_filterParams.initialize());   
 
    return StatusCode::SUCCESS;
  }


  StatusCode bbyyFilterggHBjetsAlg::execute(const EventContext& ctx)
  {
    FilterReporter filter (m_filterParams, false);
  
    SG::ReadHandle<xAOD::TruthParticleContainer> truthBottom(m_truthBottomInKey, ctx);    ATH_CHECK(truthBottom.isValid());

    if (truthBottom->empty()){
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode bbyyFilterggHBjetsAlg::finalize()
  {

    return StatusCode::SUCCESS;

  }

}
