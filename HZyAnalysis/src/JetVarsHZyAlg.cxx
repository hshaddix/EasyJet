/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetVarsHZyAlg.h"
#include <AthenaKernel/Units.h>

namespace HZY
{
  JetVarsHZyAlg::JetVarsHZyAlg(const std::string &name,
                               ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode JetVarsHZyAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("           JetVarsHZyAlg         \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetORHandle.initialize(m_systematicsList));

    // Intialise syst-aware output decorators
    for (const std::string &string_var : m_floatVariables)
    {
      CP::SysWriteDecorHandle<float> whandle{string_var + "_%SYS%", this};
      m_Fbranches.emplace(string_var, whandle);
      ATH_CHECK(m_Fbranches.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    for (const std::string &var : m_intVariables)
    {
      CP::SysWriteDecorHandle<int> whandle{var + "_%SYS%", this};
      m_Ibranches.emplace(var, whandle);
      ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode JetVarsHZyAlg::execute(const EventContext& ctx)
  {
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name() != "") continue;
      
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::JetContainer *HZyJets = nullptr;
      ANA_CHECK(m_HZyJetHandle.retrieve(HZyJets, sys, ctx));

      const xAOD::JetContainer *HZyJetsOR = nullptr;
      ANA_CHECK(m_HZyJetORHandle.retrieve(HZyJetsOR, sys, ctx));

      for (const std::string &string_var : m_floatVariables)
      {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }

      for (const auto &var : m_intVariables)
      {
        m_Ibranches.at(var).set(*event, -99, sys);
      }

      if (HZyJetsOR->size() >= 2)
      {
        TLorentzVector jet_pair = HZyJetsOR->at(0)->p4() + HZyJetsOR->at(1)->p4();
        m_Fbranches.at("Mjj").set(*event, jet_pair.M(), sys);
        m_Ibranches.at("nJets").set(*event, HZyJetsOR->size(), sys);
      }
      
    }

    return StatusCode::SUCCESS;
  }
} // namespace HZY
