/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "MultileptonFilterggHBjetsAlg.h"
#include <EventBookkeeperTools/FilterReporter.h>

namespace MULTILEPTON
{
  StatusCode MultileptonFilterggHBjetsAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("      MultileptonFilterggHBjetsAlg      \n");
    ATH_MSG_INFO("*********************************\n");

    if (!m_truthBottomInKey.empty())
      ATH_CHECK(m_truthBottomInKey.initialize());
 
    ATH_CHECK (m_filterParams.initialize());   
 
    return StatusCode::SUCCESS;
  }

  StatusCode MultileptonFilterggHBjetsAlg::execute(const EventContext& ctx)
{
    FilterReporter filter (m_filterParams, false);
    
    SG::ReadHandle<xAOD::TruthParticleContainer> truthBottom(m_truthBottomInKey, ctx);    ATH_CHECK(truthBottom.isValid());
        
    if (truthBottom->empty()){
        filter.setPassed(true);
    } else {
        filter.setPassed(false);
    }
    
    return StatusCode::SUCCESS;
}

  StatusCode MultileptonFilterggHBjetsAlg::finalize()
  {

    return StatusCode::SUCCESS;

  }

}
