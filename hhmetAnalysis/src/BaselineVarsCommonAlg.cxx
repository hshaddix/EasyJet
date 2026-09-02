/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// @author Holly Pacey

#include "BaselineVarsCommonAlg.h"
#include "AthContainers/AuxElement.h"
#include "FourMomUtils/xAODP4Helpers.h"

namespace HHMET
{
  BaselineVarsCommonAlg ::BaselineVarsCommonAlg(const std::string &name,
                                                    ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode BaselineVarsCommonAlg ::initialize()
  {

    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     BaselineVarsCommonAlg     \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    ATH_CHECK (m_genFiltMET.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_genFiltHT.initialize(m_systematicsList, m_eventHandle));
    
    // make decorators
    for (const std::string &string_var: m_genVarList) {
      CP::SysWriteDecorHandle<float> var{string_var, this};
      m_genbranches.emplace(string_var, var);
      ATH_CHECK (m_genbranches.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsCommonAlg ::execute(const EventContext& ctx)
  {
    //for (const auto& sys : m_systematicsList.systematicsVector()) {
    auto& sys = m_systematicsList.systematicsVector()[0];
      const xAOD::EventInfo *eventInfo = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (eventInfo, sys, ctx));

      if (m_isMC){
        float gfm = m_genFiltMET.get(*eventInfo, sys);
        float gfht = m_genFiltHT.get(*eventInfo, sys);
        m_genbranches.at("GenFiltMET").set(*eventInfo, gfm, sys);
        m_genbranches.at("GenFiltHT").set(*eventInfo, gfht, sys);
      }
      
    //}
    return StatusCode::SUCCESS;
  }
}
