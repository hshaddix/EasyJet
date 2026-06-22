/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "BaselineVarsHZyAlg.h"

namespace HZY
{
  BaselineVarsHZyAlg::BaselineVarsHZyAlg(const std::string &name,
                                         ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode BaselineVarsHZyAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     BaselineVarsHZyAlg      \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyPhotonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyElectronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuonHandle.initialize(m_systematicsList));

    ATH_CHECK(m_HZyPhotonORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_FsrObjectHandle.initialize(m_systematicsList));

    // Setup read decorations for Z candidate dilepton variables (float)
    for (const std::string &string_var : m_floatVariables_leptons)
    {
      CP::SysReadDecorHandle<float> var{string_var + "_%SYS%", this};
      m_Fbranches_leptons.emplace(string_var, var);
      ATH_CHECK(m_Fbranches_leptons.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    // Intialise syst-aware output decorators
    for (const std::string &var : m_floatVariables)
    {
      CP::SysWriteDecorHandle<float> whandle{var + "_%SYS%", this};
      m_Fbranches.emplace(var, whandle);
      ATH_CHECK(
          m_Fbranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    for (const std::string &var : m_intVariables)
    {
      CP::SysWriteDecorHandle<int> whandle{var + "_%SYS%", this};
      m_Ibranches.emplace(var, whandle);
      ATH_CHECK(
          m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsHZyAlg::execute(const EventContext& /*ctx*/)
  {
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name() != "") continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys));

      const xAOD::PhotonContainer *HZyPhotons = nullptr;
      ANA_CHECK(m_HZyPhotonHandle.retrieve(HZyPhotons, sys));

      const xAOD::ElectronContainer *HZyElectrons = nullptr;
      ANA_CHECK(m_HZyElectronHandle.retrieve(HZyElectrons, sys));

      const xAOD::MuonContainer *HZyMuons = nullptr;
      ANA_CHECK(m_HZyMuonHandle.retrieve(HZyMuons, sys));

      const xAOD::JetContainer *HZyJets = nullptr;
      ANA_CHECK(m_HZyJetHandle.retrieve(HZyJets, sys));

      const xAOD::PhotonContainer *HZyPhotonOR = nullptr;
      ANA_CHECK(m_HZyPhotonORHandle.retrieve(HZyPhotonOR, sys));

      const xAOD::IParticleContainer* fsrObject = nullptr;
      ANA_CHECK(m_FsrObjectHandle.retrieve(fsrObject, sys));

      // Reset all decorations to default values at the start of each systematics loop
      for (const std::string &string_var : m_floatVariables)
      {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }
      for (const auto &var : m_intVariables)
      {
        m_Ibranches.at(var).set(*event, -99, sys);
      }

      // PhPy8EG_PDF4LHC21_ggH_NNLOPS with DSID 603798: nominal weight is at index 88
      const auto &w = event->mcEventWeights();
      size_t i = event->mcChannelNumber() == 603798 ? 88u : 0u;
      m_Fbranches.at("MC_Weight").set(*event, w.size() > i ? w[i] : (w.empty() ? 1.0f : w[0]), sys);
      m_Fbranches.at("BeamSpotWeight").set(*event, event->beamSpotWeight(), sys);

      // Reconstruct dilepton and photon final state
      TLorentzVector llRawP4;
      llRawP4.SetPtEtaPhiM(
        m_Fbranches_leptons.at("LepLepZ_Pt_Raw").get(*event, sys),
        m_Fbranches_leptons.at("LepLepZ_Eta_Raw").get(*event, sys),
        m_Fbranches_leptons.at("LepLepZ_Phi_Raw").get(*event, sys),
        m_Fbranches_leptons.at("LepLepZ_M_Raw").get(*event, sys));
      TLorentzVector llCstrP4;
      llCstrP4.SetPtEtaPhiM(
        m_Fbranches_leptons.at("LepLepZ_Pt_Cstr").get(*event, sys),
        m_Fbranches_leptons.at("LepLepZ_Eta_Cstr").get(*event, sys),
        m_Fbranches_leptons.at("LepLepZ_Phi_Cstr").get(*event, sys),
        m_Fbranches_leptons.at("LepLepZ_M_Cstr").get(*event, sys));

      const xAOD::Photon* selHZyLoosePhoton =
        (HZyPhotonOR && !HZyPhotonOR->empty()) ? HZyPhotonOR->at(0) : nullptr;
      if (selHZyLoosePhoton){
        const TLorentzVector llgRawP4 = llRawP4 + selHZyLoosePhoton->p4();
        m_Fbranches.at("LLGam_M_Raw").set(*event, llgRawP4.M(), sys);
        m_Fbranches.at("LLGam_Pt_Raw").set(*event, llgRawP4.Pt(), sys);
        m_Fbranches.at("LLGam_Eta_Raw").set(*event, llgRawP4.Eta(), sys);
        m_Fbranches.at("LLGam_Phi_Raw").set(*event, llgRawP4.Phi(), sys);

        const xAOD::IParticle *fsr =
            (fsrObject && !fsrObject->empty()) ? fsrObject->at(0) : nullptr;
        TLorentzVector fsrP4;
        if (fsr){
          fsrP4 = fsr->p4();
        }
        const TLorentzVector llgFsrP4 = llRawP4 + fsrP4 + selHZyLoosePhoton->p4();
        m_Fbranches.at("LLGam_M_Fsr").set(*event, llgFsrP4.M(), sys);

        const TLorentzVector llgCstrP4 = llCstrP4 + selHZyLoosePhoton->p4();
        m_Fbranches.at("LLGam_M_Cstr").set(*event, llgCstrP4.M(), sys);
        m_Fbranches.at("LLGam_Pt_Cstr").set(*event, llgCstrP4.Pt(), sys);
        m_Fbranches.at("LLGam_Eta_Cstr").set(*event, llgCstrP4.Eta(), sys);
        m_Fbranches.at("LLGam_Phi_Cstr").set(*event, llgCstrP4.Phi(), sys);
        m_Fbranches.at("LLGam_RelPhpT_Cstr").set(*event, selHZyLoosePhoton->pt() / llgCstrP4.M(), sys);
      }
      else {
        ATH_MSG_DEBUG("No cleaned photon found, skipping photon variables.");
      }
    }

    return StatusCode::SUCCESS;
  }
} // namespace HZY
