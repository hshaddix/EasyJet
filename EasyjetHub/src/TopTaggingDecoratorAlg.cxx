/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// @author JaeJin Hong

#include "TopTaggingDecoratorAlg.h"

namespace Easyjet
{
  StatusCode TopTaggingDecoratorAlg ::initialize()
  {
    ATH_CHECK (m_jetsInKey.initialize());

    ATH_CHECK (m_Toptagger.retrieve());
    
    return StatusCode::SUCCESS;
  }

  StatusCode TopTaggingDecoratorAlg ::execute(const EventContext& ctx) 
  {

    SG::ReadHandle<xAOD::JetContainer> LRjets(m_jetsInKey, ctx);
    ATH_CHECK (LRjets.isValid());
    ATH_MSG_DEBUG("Number of LR jets: " << LRjets->size());
    ATH_CHECK( m_Toptagger->decorate( *LRjets ));
    return StatusCode::SUCCESS;
    
  }
}
