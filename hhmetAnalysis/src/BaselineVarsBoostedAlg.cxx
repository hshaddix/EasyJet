/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// @author Holly Pacey

#include "BaselineVarsBoostedAlg.h"
#include "AthContainers/AuxElement.h"
#include "FourMomUtils/xAODP4Helpers.h"

namespace HHMET
{
  StatusCode BaselineVarsBoostedAlg ::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("      BaselineVarsBoostedAlg     \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK (m_largeRjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_smallRjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    ATH_CHECK(m_R10TruthLabel.initialize(m_systematicsList, m_largeRjetHandle));
    ATH_CHECK(m_Tau2_wta.initialize(m_systematicsList, m_largeRjetHandle));
    ATH_CHECK(m_Tau3_wta.initialize(m_systematicsList, m_largeRjetHandle));

    // make decorators
    for (const std::string &var: m_vars)
    {
      std::string deco_var = var + "_" + m_bTagWP;
      CP::SysWriteDecorHandle<float> deco{deco_var+"_%SYS%", this};
      m_decos.emplace(deco_var, deco);
      ATH_CHECK (m_decos.at(deco_var).initialize(m_systematicsList, m_eventHandle));
    }
    for (const std::string &var: m_largeR_vars)
    {
      CP::SysWriteDecorHandle<float> deco{var+"_%SYS%", this};
      m_decos.emplace(var, deco);
      ATH_CHECK (m_decos.at(var).initialize(m_systematicsList, m_eventHandle));
    }
    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsBoostedAlg ::execute(const EventContext& ctx)
  {
    for (const auto& sys : m_systematicsList.systematicsVector()) {
      // container we read in
      const xAOD::EventInfo *eventInfo = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (eventInfo, sys, ctx));

      const xAOD::JetContainer *smallRjets = nullptr;
      ANA_CHECK (m_smallRjetHandle.retrieve (smallRjets, sys, ctx));
      std::size_t n_smallRjets = smallRjets->size();

      const xAOD::JetContainer *largeRjets = nullptr;
      ANA_CHECK (m_largeRjetHandle.retrieve (largeRjets, sys, ctx));
      std::size_t n_largeRjets = largeRjets->size();

      // set defaults

      for (const std::string& var : m_vars)
      {
        std::string deco_var = var + "_" + m_bTagWP;
        m_decos.at(deco_var).set(*eventInfo, -10, sys);
      }
      for (const std::string& var : m_largeR_vars)
      {
        m_decos.at(var).set(*eventInfo, -10, sys);
      }

      // large jet sector
      if (n_largeRjets >= 1 && n_smallRjets >=2)
      {
        TLorentzVector h1 = largeRjets->at(0)->p4();
        TLorentzVector h2 = smallRjets->at(0)->p4() + smallRjets->at(1)->p4();

        m_decos.at("boosted_h1_m").set(*eventInfo, h1.M(), sys);
        m_decos.at("boosted_h1_pt").set(*eventInfo, h1.Pt(), sys);
        m_decos.at("boosted_h1_eta").set(*eventInfo, h1.Eta(), sys);
        m_decos.at("boosted_h1_phi").set(*eventInfo, h1.Phi(), sys);
        m_decos.at("boosted_h1_E").set(*eventInfo, h1.E(), sys);

        m_decos.at("boosted_h2_m_" + m_bTagWP).set(*eventInfo, h2.M(), sys);
        m_decos.at("boosted_h2_pt_" + m_bTagWP).set(*eventInfo, h2.Pt(), sys);
        m_decos.at("boosted_h2_eta_" + m_bTagWP).set(*eventInfo, h2.Eta(), sys);
        m_decos.at("boosted_h2_phi_" + m_bTagWP).set(*eventInfo, h2.Phi(), sys);
        m_decos.at("boosted_h2_E_" + m_bTagWP).set(*eventInfo, h2.E(), sys);

        TLorentzVector hh = h1 + h2;
        m_decos.at("boosted_hh_m_" + m_bTagWP).set(*eventInfo, hh.M(), sys);
        m_decos.at("boosted_hh_pt_" + m_bTagWP).set(*eventInfo, hh.Pt(), sys);
        m_decos.at("boosted_hh_deltaEta_" + m_bTagWP).set(*eventInfo, h1.Eta() - h2.Eta(), sys);
        m_decos.at("boosted_hh_deltaPhi_" + m_bTagWP).set(*eventInfo, h1.DeltaPhi(h2), sys);

        std::string prefix = "boosted_h1";

        float tau32 = m_Tau3_wta.get(*largeRjets->at(0),sys) /
            m_Tau2_wta.get(*largeRjets->at(0),sys);
        m_decos.at(prefix+"_Tau32_wta").set(*eventInfo, tau32, sys);

        if (m_isMC){
          int truthLabel_i = m_R10TruthLabel.get(*largeRjets->at(0), sys);
          m_decos.at(prefix+"_truthLabel").set(*eventInfo, truthLabel_i, sys);
        }

      }
    }
    return StatusCode::SUCCESS;
  }
}
