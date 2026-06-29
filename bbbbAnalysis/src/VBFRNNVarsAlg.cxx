/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// @author Frederic Renner

#include "VBFRNNVarsAlg.h"
#include "AthContainers/AuxElement.h"

namespace HH4B
{
  VBFRNNVarsAlg ::VBFRNNVarsAlg(const std::string &name,
                                                  ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode VBFRNNVarsAlg ::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("      VBFRNNVarsAlg     \n");
    ATH_MSG_INFO("*********************************\n");

    if(m_UseVBFRNN){
      if(m_doBoosted){
        ATH_CHECK (m_RNNjetBoostedHandle.initialize(m_systematicsList));
      }
      if(m_doResolved){
        ATH_CHECK (m_RNNjetResolvedHandle.initialize(m_systematicsList));
      }
    }
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    // make decorators — construct directly in map to avoid double init
    for (const std::string &string_var: m_Fvars.value()) {
      auto [it, inserted] = m_Fdecos.try_emplace(
          string_var, string_var + "_%SYS%", this);
      if (inserted) {
        ATH_CHECK (it->second.initialize(m_systematicsList, m_eventHandle));
      }
    }

    // Pre-build decoration key lookup tables
    const std::array<std::string, 4> suffixes = {"_m", "_pt", "_eta", "_phi"};
    for (unsigned int j = 0; j < 2; ++j) {
      std::string jetStr = "_Jet" + std::to_string(j + 1);
      for (unsigned int k = 0; k < 4; ++k) {
        m_boostedDecoKeys[j][k] =
            m_boostedPrefix.value() + jetStr + suffixes[k];
        m_resolvedDecoKeys[j][k] =
            m_resolvedPrefix.value() + jetStr + suffixes[k];
      }
    }

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode VBFRNNVarsAlg ::execute(const EventContext& ctx)
  {
    for (const auto& sys : m_systematicsList.systematicsVector()) {
      // container we read in
      const xAOD::EventInfo *eventInfo = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (eventInfo, sys, ctx));

      const xAOD::JetContainer *RNNJets_boosted= nullptr;
      const xAOD::JetContainer *RNNJets_resolved= nullptr;
      if(m_UseVBFRNN) {
        if(m_doBoosted){
          ANA_CHECK (m_RNNjetBoostedHandle.retrieve (RNNJets_boosted, sys, ctx));
        }
        if(m_doResolved){
          ANA_CHECK (m_RNNjetResolvedHandle.retrieve (RNNJets_resolved, sys, ctx));
        }
      }

      // set defaults
      for (const std::string& var : m_Fvars) {
        m_Fdecos.at(var).set(*eventInfo, -99, sys);
      }

      // boosted RNN jets
      if (m_UseVBFRNN && m_doBoosted){
        for(unsigned int j=0; j<std::min(size_t(2),RNNJets_boosted->size()); j++) {
          const xAOD::Jet* RNNJet = RNNJets_boosted->at(j);
          m_Fdecos.at(m_boostedDecoKeys[j][0]).set(*eventInfo, RNNJet->m(), sys);
          m_Fdecos.at(m_boostedDecoKeys[j][1]).set(*eventInfo, RNNJet->pt(), sys);
          m_Fdecos.at(m_boostedDecoKeys[j][2]).set(*eventInfo, RNNJet->eta(), sys);
          m_Fdecos.at(m_boostedDecoKeys[j][3]).set(*eventInfo, RNNJet->phi(), sys);
        }
      } // end of boosted RNN jets

      // resolved RNN jets
      if (m_UseVBFRNN && m_doResolved){
        for(unsigned int j=0; j<std::min(size_t(2),RNNJets_resolved->size()); j++) {
          const xAOD::Jet* RNNJet = RNNJets_resolved->at(j);
          m_Fdecos.at(m_resolvedDecoKeys[j][0]).set(*eventInfo, RNNJet->m(), sys);
          m_Fdecos.at(m_resolvedDecoKeys[j][1]).set(*eventInfo, RNNJet->pt(), sys);
          m_Fdecos.at(m_resolvedDecoKeys[j][2]).set(*eventInfo, RNNJet->eta(), sys);
          m_Fdecos.at(m_resolvedDecoKeys[j][3]).set(*eventInfo, RNNJet->phi(), sys);
        }
      } // end of resolved RNN jets
    }
    return StatusCode::SUCCESS;
  }
}
