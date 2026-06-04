/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "PhotonVarsHZyAlg.h"


namespace HZY
{
  PhotonVarsHZyAlg::PhotonVarsHZyAlg(const std::string &name,
                                     ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode PhotonVarsHZyAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("         PhotonVarsHZyAlg        \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyPhotonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_photonTIWPDecorHandle.initialize(m_systematicsList, m_HZyPhotonHandle));
    ATH_CHECK(m_photon_SF.initialize(m_systematicsList, m_HZyPhotonHandle));
    ATH_CHECK(m_isEMTight.initialize(m_systematicsList, m_HZyPhotonHandle));

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

  StatusCode PhotonVarsHZyAlg::execute()
  {
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name() != "") continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys));

      const xAOD::PhotonContainer *photons = nullptr;
      ANA_CHECK(m_HZyPhotonHandle.retrieve(photons, sys));

      // Set default values for all output branches, in case we have fewer photons than expected
      for (const std::string &string_var : m_floatVariables)
      {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }

      for (const auto &var : m_intVariables)
      {
        m_Ibranches.at(var).set(*event, -99, sys);
      }

      const xAOD::Photon *signalPhoton = nullptr;
      for (const xAOD::Photon *photon : *photons)
      {
        if (!photon || !m_photonTIWPDecorHandle.get(*photon, sys))
          continue;

        if (!signalPhoton || photon->pt() > signalPhoton->pt())
          signalPhoton = photon;
      }
      
      float Photon_SF = 1.0;
      if (signalPhoton)
      {
        if (m_isMC) {
          Photon_SF = m_photon_SF.get(*signalPhoton, sys);
        }

        m_Ibranches.at("Ph_ConvType").set(*event, signalPhoton->conversionType(), sys);
        m_Fbranches.at("Ph_Pt_Raw").set(*event, signalPhoton->pt() / Athena::Units::GeV, sys);
        m_Fbranches.at("Ph_Eta_Raw").set(*event, signalPhoton->eta(), sys);
        m_Fbranches.at("Ph_Phi_Raw").set(*event, signalPhoton->phi(), sys);
        m_Ibranches.at("Ph_isEMTight").set(*event, m_isEMTight.get(*signalPhoton, sys), sys);
      }

      m_Ibranches.at("nPhotons").set(*event, photons->size(), sys);
      m_Fbranches.at("Photon_effSF").set(*event, Photon_SF, sys);

    }

    return StatusCode::SUCCESS;
  }
} // namespace HZY
