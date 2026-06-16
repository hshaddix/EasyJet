/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "PhotonVarsHZyAlg.h"
#include "HZyCommonUtils.h"

// Include xAOD headers for const_cast and containers
#include <AthContainers/ConstDataVector.h>
#include <xAODEgamma/Photon.h>
#include <xAODJet/Jet.h>
#include <xAODMuon/Muon.h>
#include <xAODMuon/MuonContainer.h>

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
    ATH_CHECK(m_HZyJetHandle.initialize(m_systematicsList));

    ATH_CHECK(m_ZCandElectronsHandle.initialize(m_systematicsList));
    ATH_CHECK(m_ZCandMuonsHandle.initialize(m_systematicsList));

    ATH_CHECK(m_HZyPhotonORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetORHandle.initialize(m_systematicsList));

    ATH_CHECK(m_photonTIdLIsoDecorHandle.initialize(m_systematicsList, m_HZyPhotonHandle));
    ATH_CHECK(m_ph_SF.initialize(m_systematicsList, m_HZyPhotonHandle));
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

      const xAOD::PhotonContainer *HZyPhotons = nullptr;
      ANA_CHECK(m_HZyPhotonHandle.retrieve(HZyPhotons, sys));

      const xAOD::JetContainer *HZyJets = nullptr;
      ANA_CHECK(m_HZyJetHandle.retrieve(HZyJets, sys));

      const xAOD::ElectronContainer *HZyElectronsOR_ZCand = nullptr;
      ANA_CHECK(m_ZCandElectronsHandle.retrieve(HZyElectronsOR_ZCand, sys));

      const xAOD::MuonContainer *HZyMuonsOR_ZCand = nullptr;
      ANA_CHECK(m_ZCandMuonsHandle.retrieve(HZyMuonsOR_ZCand, sys));

      // Create mutable views of the photon and jet containers to be decorated with OR decisions
      auto photonsView = std::make_unique<ConstDataVector<xAOD::PhotonContainer>>(SG::VIEW_ELEMENTS);
      for (const xAOD::Photon *ph : *HZyPhotons)
        photonsView->push_back(ph);

      auto jetsView = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
      for (const xAOD::Jet *jet : *HZyJets)
        jetsView->push_back(jet);

      auto caloZCandMuonsView = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
      auto nonCaloZCandMuonsView = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
      
      if (HZyMuonsOR_ZCand){
        for (const xAOD::Muon *muon : *HZyMuonsOR_ZCand)
        {
          if (muon->muonType() == xAOD::Muon::MuonType::CaloTagged)
            caloZCandMuonsView->push_back(muon);
          else
            nonCaloZCandMuonsView->push_back(muon);
        }
      }

      ATH_MSG_DEBUG("Starting photon OR against Z-candidate leptons.");
      // Photon/Electron OR (Remove Photon if near Z-Cand Electron)
      safeRemoveOverlap(*photonsView, *HZyElectronsOR_ZCand, m_dR_Ph_El);
      // Photon/Calo Muon OR (Remove Photon if near Z-Cand Calo Muon)
      safeRemoveOverlap(*photonsView, *caloZCandMuonsView, m_dR_Ph_CaloMu);
      // Photon/Staco Muon OR (Remove Photon if near Z-Cand non-Calo Muon)
      safeRemoveOverlap(*photonsView, *nonCaloZCandMuonsView, m_dR_Ph_StacoMu);

      ATH_MSG_DEBUG("Starting jet OR against cleaned photons and Z-candidate leptons.");
      // Jet/Photon OR (Remove Jet if near Photon - requires the already-cleaned photon container)
      safeRemoveOverlap(*jetsView, *photonsView, m_dR_Jet_Ph);
      // Jet/Electron OR (Remove Jet if near Z-Cand Electron)
      safeRemoveOverlap(*jetsView, *HZyElectronsOR_ZCand, m_dR_Jet_El);
      // Jet/Calo Muon OR (Remove Jet if near Z-Cand Calo Muon)
      safeRemoveOverlap(*jetsView, *caloZCandMuonsView, m_dR_Jet_CaloMu);
      // Jet/Staco Muon OR (Remove Jet if near Z-Cand Staco Muon)
      safeRemoveOverlap(*jetsView, *nonCaloZCandMuonsView, m_dR_Jet_StacoMu);
      
      // Set default values for all output branches, in case we have fewer photons than expected
      for (const std::string &string_var : m_floatVariables)
      {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }

      for (const auto &var : m_intVariables)
      {
        m_Ibranches.at(var).set(*event, -99, sys);
      }

      ATH_MSG_DEBUG("Before recording OR views: photonsView size = "
        << photonsView->size() << ", jetsView size = " << jetsView->size());
      const xAOD::Photon *signalPhoton = nullptr;
      for (const xAOD::Photon *photon : *photonsView)
      {
        if (!photon || !m_photonTIdLIsoDecorHandle.get(*photon, sys))
          continue;

        if (!signalPhoton || photon->pt() > signalPhoton->pt())
          signalPhoton = photon;
      }
      
      float Photon_SF = 1.0;
      if (signalPhoton)
      {
        if (m_isMC) {
          Photon_SF = m_ph_SF.get(*signalPhoton, sys);
        }

        m_Ibranches.at("Ph_ConvType").set(*event, signalPhoton->conversionType(), sys);
        m_Fbranches.at("Ph_Pt_Raw").set(*event, signalPhoton->pt() / Athena::Units::GeV, sys);
        m_Fbranches.at("Ph_Eta_Raw").set(*event, signalPhoton->eta(), sys);
        m_Fbranches.at("Ph_Phi_Raw").set(*event, signalPhoton->phi(), sys);
        m_Ibranches.at("Ph_isEMTight").set(*event, m_isEMTight.get(*signalPhoton, sys), sys);
      }

      m_Ibranches.at("nPhotons").set(*event, HZyPhotons->size(), sys);
      m_Fbranches.at("Photon_effSF").set(*event, Photon_SF, sys);

      ATH_CHECK(m_HZyPhotonORHandle.record(std::move(photonsView), sys));
      ATH_CHECK(m_HZyJetORHandle.record(std::move(jetsView), sys));

    }

    return StatusCode::SUCCESS;
  }
} // namespace HZY
