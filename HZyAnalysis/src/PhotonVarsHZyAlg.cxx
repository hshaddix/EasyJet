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
  StatusCode PhotonVarsHZyAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("         PhotonVarsHZyAlg        \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyPhotonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyPhotonORInHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetHandle.initialize(m_systematicsList));

    ATH_CHECK(m_ZCandElectronsHandle.initialize(m_systematicsList));
    ATH_CHECK(m_ZCandMuonsHandle.initialize(m_systematicsList));

    ATH_CHECK(m_HZyElectronORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuonORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyElORPhotonsHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuORPhotonsHandle.initialize(m_systematicsList));

    ATH_CHECK(m_HZyPhotonORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetORNoJvtHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyElORPhotonsExtraHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuORPhotonsExtraHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetORExtraHandle.initialize(m_systematicsList));

    ATH_CHECK(m_photonTIdNonIsoDecorHandle.initialize(m_systematicsList, m_HZyPhotonHandle));
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

  StatusCode PhotonVarsHZyAlg::execute(const EventContext& ctx)
  {
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name() != "") continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::PhotonContainer *HZyPhotons = nullptr;
      ANA_CHECK(m_HZyPhotonHandle.retrieve(HZyPhotons, sys, ctx));

      // Photons already OR'd against Z-cand leptons (produced by LeptonVarsHZyAlg)
      const xAOD::PhotonContainer *HZyPhotonsOR_Lep = nullptr;
      ANA_CHECK(m_HZyPhotonORInHandle.retrieve(HZyPhotonsOR_Lep, sys, ctx));

      const xAOD::JetContainer *HZyJets = nullptr;
      ANA_CHECK(m_HZyJetHandle.retrieve(HZyJets, sys, ctx));

      const xAOD::ElectronContainer *HZyElectronsOR_ZCand = nullptr;
      ANA_CHECK(m_ZCandElectronsHandle.retrieve(HZyElectronsOR_ZCand, sys, ctx));

      const xAOD::MuonContainer *HZyMuonsOR_ZCand = nullptr;
      ANA_CHECK(m_ZCandMuonsHandle.retrieve(HZyMuonsOR_ZCand, sys, ctx));

      const xAOD::ElectronContainer *HZyElectronOR = nullptr;
      ANA_CHECK(m_HZyElectronORHandle.retrieve(HZyElectronOR, sys, ctx));

      const xAOD::MuonContainer *HZyMuonOR = nullptr;
      ANA_CHECK(m_HZyMuonORHandle.retrieve(HZyMuonOR, sys, ctx));

      // Set default values for all output branches, in case we have fewer photons than expected
      for (const std::string &string_var : m_floatVariables)
      {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }

      for (const auto &var : m_intVariables)
      {
        m_Ibranches.at(var).set(*event, -99, sys);
      }

      // Create mutable view from photons already OR'd against Z-cand leptons (by LeptonVarsHZyAlg)
      auto photonsView = std::make_unique<ConstDataVector<xAOD::PhotonContainer>>(SG::VIEW_ELEMENTS);
      for (const xAOD::Photon *ph : *HZyPhotonsOR_Lep)
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

      // Photon OR against Z-candidate leptons is done upstream by LeptonVarsHZyAlg.
      m_Ibranches.at("nPhotonsOR_LIdNIso").set(*event, static_cast<int>(photonsView->size()), sys);

      // Decorate photon variables for the leading photon passing OR, if it exists
      const xAOD::Photon *selHZyLoosePhoton =
          (photonsView && !photonsView->empty()) ? photonsView->asDataVector()->at(0) : nullptr;
      const bool passTightIDNonIso =
          selHZyLoosePhoton ? m_photonTIdNonIsoDecorHandle.get(*selHZyLoosePhoton, sys) : false;
      const bool passTightIDLooseIso =
          selHZyLoosePhoton ? m_photonTIdLIsoDecorHandle.get(*selHZyLoosePhoton, sys) : false;

      m_Ibranches.at("nPhotonsOR_TIdNIso").set(*event, passTightIDNonIso ? 1 : 0, sys);
      m_Ibranches.at("nPhotonsOR_TIdLIso").set(*event, passTightIDLooseIso ? 1 : 0, sys);

      ATH_MSG_DEBUG("Before recording OR views: photonsView size = "
        << photonsView->size() << ", jetsView size = " << jetsView->size());
      
      // lead photon as candidate
      const xAOD::Photon *signalPhoton = nullptr;
      signalPhoton = selHZyLoosePhoton;
      
      float Photon_SF = 1.0;
      if (signalPhoton)
      {
        if (m_isMC) {
          Photon_SF = m_ph_SF.get(*signalPhoton, sys);
        }

        m_Ibranches.at("Ph_ConvType").set(*event, signalPhoton->conversionType(), sys);
        m_Fbranches.at("Ph_Pt_Raw").set(*event, signalPhoton->pt(), sys);
        m_Fbranches.at("Ph_Eta_Raw").set(*event, signalPhoton->eta(), sys);
        m_Fbranches.at("Ph_Phi_Raw").set(*event, signalPhoton->phi(), sys);
        m_Ibranches.at("Ph_isEMTight").set(*event, m_isEMTight.get(*signalPhoton, sys), sys);

        // Isolation variables — use a single availability check per group
        static const SG::ConstAccessor<float> acc_ptcone20("ptcone20");
        static const SG::ConstAccessor<float> acc_Reta("Reta");
        if (acc_ptcone20.isAvailable(*signalPhoton)) {
          m_Fbranches.at("Ph_topoetcone20").set(*event,
              signalPhoton->isolation(xAOD::Iso::topoetcone20), sys);
          m_Fbranches.at("Ph_topoetcone30").set(*event,
              signalPhoton->isolation(xAOD::Iso::topoetcone30), sys);
          m_Fbranches.at("Ph_topoetcone40").set(*event,
              signalPhoton->isolation(xAOD::Iso::topoetcone40), sys);
          m_Fbranches.at("Ph_ptcone20").set(*event,
              signalPhoton->isolation(xAOD::Iso::ptcone20), sys);
        }

        // Shower shape variables — one check for all
        if (acc_Reta.isAvailable(*signalPhoton)) {
          m_Fbranches.at("Ph_Reta").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::Reta), sys);
          m_Fbranches.at("Ph_Rhad").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::Rhad), sys);
          m_Fbranches.at("Ph_Rhad1").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::Rhad1), sys);
          m_Fbranches.at("Ph_Rphi").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::Rphi), sys);
          m_Fbranches.at("Ph_f1").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::f1), sys);
          m_Fbranches.at("Ph_f3").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::f3), sys);
          m_Fbranches.at("Ph_weta1").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::weta1), sys);
          m_Fbranches.at("Ph_weta2").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::weta2), sys);
          m_Fbranches.at("Ph_wtots1").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::wtots1), sys);
          m_Fbranches.at("Ph_Eratio").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::Eratio), sys);
          m_Fbranches.at("Ph_fracs1").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::fracs1), sys);
          m_Fbranches.at("Ph_DeltaE").set(*event,
              signalPhoton->showerShapeValue(xAOD::EgammaParameters::DeltaE), sys);
        }

      } // end if (signalPhoton)

      m_Ibranches.at("nPhotons_LIdNIso").set(*event, static_cast<int>(HZyPhotons->size()), sys);
      m_Fbranches.at("Photon_effSF").set(*event, Photon_SF, sys);

      // ------------------------------------------------------------
      // Create mutable views of the full cleaned electron/muon
      // collections. These are the equivalent of old `electrons`
      // and `muons` before their final OR with the selected photon.
      // ------------------------------------------------------------
      auto electronsORPhotonsView = std::make_unique<ConstDataVector<xAOD::ElectronContainer>>(SG::VIEW_ELEMENTS);
      for (const xAOD::Electron* electron : *HZyElectronOR)
      {
        if (electron)
          electronsORPhotonsView->push_back(electron);
      }

      auto muonsORPhotonsView = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
      for (const xAOD::Muon* muon : *HZyMuonOR)
      {
        if (muon)
          muonsORPhotonsView->push_back(muon);
      }

      // ------------------------------------------------------------
      // lepton-photon OR to find extra leptons after the signal photon selection
      // remove electrons/muons overlapping with the one selected signal photon.
      // ------------------------------------------------------------
      if (signalPhoton)
      {
        auto signalPhotonView = std::make_unique<ConstDataVector<xAOD::PhotonContainer>>(SG::VIEW_ELEMENTS);
        signalPhotonView->push_back(signalPhoton);

        safeRemoveOverlap(*electronsORPhotonsView, *signalPhotonView, m_dR_Ph_El);

        auto caloMuonsORPhotonsView = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
        auto nonCaloMuonsORPhotonsView = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
        for (const xAOD::Muon* muon : *muonsORPhotonsView)
        {
          if (!muon) continue;
          if (muon->muonType() == xAOD::Muon::MuonType::CaloTagged)
            caloMuonsORPhotonsView->push_back(muon);
          else
            nonCaloMuonsORPhotonsView->push_back(muon);
        }

        safeRemoveOverlap(*caloMuonsORPhotonsView, *signalPhotonView, m_dR_Ph_CaloMu);
        safeRemoveOverlap(*nonCaloMuonsORPhotonsView, *signalPhotonView, m_dR_Ph_StacoMu);

        muonsORPhotonsView = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
        for (const xAOD::Muon* muon : *caloMuonsORPhotonsView)
          muonsORPhotonsView->push_back(muon);

        for (const xAOD::Muon* muon : *nonCaloMuonsORPhotonsView)
          muonsORPhotonsView->push_back(muon);
      }
      m_Ibranches.at("nElectronsIdIso_ORPh").set(*event, static_cast<int>(electronsORPhotonsView->size()), sys);
      m_Ibranches.at("nMuonsIdIso_ORPh").set(*event, static_cast<int>(muonsORPhotonsView->size()), sys);

      // ------------------------------------------------------------
      // Build extra-lepton containers (Z-cand leptons + highest-pT
      // extra lepton) and HZyJetsOR_Extra (jets OR + extra lepton OR).
      // Must run BEFORE recording jetsView/electronsORPhotonsView
      // since those use std::move.
      // note: this could overlap with 4-lepton selection in LeptonVarsHZyAlg
      // but we have 4-lepton tag and can deal with it offline
      // ------------------------------------------------------------
      auto inZCand = [](const xAOD::IParticle *p,
                        const xAOD::ElectronContainer *zcEl,
                        const xAOD::MuonContainer *zcMu) -> bool {
        if (!p) return false;
        if (zcEl) {
          for (const auto *el : *zcEl)
            if (el == p) return true;
        }
        if (zcMu) {
          for (const auto *mu : *zcMu)
            if (mu == p) return true;
        }
        return false;
      };

      struct ExtraLep {
        const xAOD::IParticle *p = nullptr;
        int label = 0; // 1=electron, 2=muon
      };
      std::vector<ExtraLep> extras;
      for (const xAOD::Electron *el : *electronsORPhotonsView) {
        if (!inZCand(el, HZyElectronsOR_ZCand, nullptr))
          extras.push_back({el, 1});
      }
      for (const xAOD::Muon *mu : *muonsORPhotonsView) {
        if (!inZCand(mu, nullptr, HZyMuonsOR_ZCand))
          extras.push_back({mu, 2});
      }

      const xAOD::IParticle *bestExtra = nullptr;
      int extraLabel = 0;
      for (const auto &ex : extras) {
        if (!bestExtra || (ex.p && ex.p->pt() > bestExtra->pt())) {
          bestExtra = ex.p;
          extraLabel = ex.label;
        }
      }
      
      // build the el/muon container for trigger match
      auto elExtraView = std::make_unique<ConstDataVector<xAOD::ElectronContainer>>(SG::VIEW_ELEMENTS);
      if (HZyElectronsOR_ZCand) {
        for (const auto *el : *HZyElectronsOR_ZCand)
          elExtraView->push_back(el);
      }
      if (extraLabel == 1 && bestExtra)
        elExtraView->push_back(dynamic_cast<const xAOD::Electron *>(bestExtra));

      auto muExtraView = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
      if (HZyMuonsOR_ZCand) {
        for (const auto *mu : *HZyMuonsOR_ZCand)
          muExtraView->push_back(mu);
      }
      if (extraLabel == 2 && bestExtra)
        muExtraView->push_back(dynamic_cast<const xAOD::Muon *>(bestExtra));
      
      //save ExtraLep p4
      if (bestExtra) {
        m_Fbranches.at("ExtraLep_Pt").set(*event, bestExtra->pt(), sys);
        m_Fbranches.at("ExtraLep_Eta").set(*event, bestExtra->eta(), sys);
        m_Fbranches.at("ExtraLep_Phi").set(*event, bestExtra->phi(), sys);
        m_Ibranches.at("ExtraLep_Label").set(*event, extraLabel, sys);
      }
      
      // jets overlap removal, first remove with 
      ATH_MSG_DEBUG("Starting jet OR against cleaned photons and Z-candidate leptons.");
      // Jet/Photon OR (Remove Jet if near Photon - requires the already-cleaned photon container)
      safeRemoveOverlap(*jetsView, *photonsView, m_dR_Jet_Ph);
      // Jet/Electron OR (Remove Jet if near Z-Cand Electron)
      safeRemoveOverlap(*jetsView, *HZyElectronsOR_ZCand, m_dR_Jet_El);
      // Jet/Calo Muon OR (Remove Jet if near Z-Cand Calo Muon)
      safeRemoveOverlap(*jetsView, *caloZCandMuonsView, m_dR_Jet_CaloMu);
      // Jet/Staco Muon OR (Remove Jet if near Z-Cand Staco Muon)
      safeRemoveOverlap(*jetsView, *nonCaloZCandMuonsView, m_dR_Jet_StacoMu);
      // Build jetsORExtraView from jetsView (still alive) + extra lepton OR
      auto jetsORExtraView = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
      for (const xAOD::Jet *jet : *jetsView)
        jetsORExtraView->push_back(jet);

      if (bestExtra) {
        if (extraLabel == 1) {
          auto el = dynamic_cast<const xAOD::Electron*>(bestExtra);
          if (el) {
            auto v = std::make_unique<ConstDataVector<xAOD::ElectronContainer>>(SG::VIEW_ELEMENTS);
            v->push_back(el);
            safeRemoveOverlap(*jetsORExtraView, *v, m_dR_Jet_El);
          }
        } else if (extraLabel == 2) {
          auto mu = dynamic_cast<const xAOD::Muon*>(bestExtra);
          if (mu) {
            auto v = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
            v->push_back(mu);
            safeRemoveOverlap(*jetsORExtraView, *v,
              mu->muonType() == xAOD::Muon::CaloTagged ? m_dR_Jet_CaloMu : m_dR_Jet_StacoMu);
          }
        }
      }

      // Save jetsORExtraView before JVT cut as HZyJetsOR_NoJvt
      // (used for HZy-specific JVT event SF computation, no LooseBad filter)
      auto jetsORNoJvtView = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
      for (const xAOD::Jet *jet : *jetsORExtraView)
        jetsORNoJvtView->push_back(jet);

      // Apply LooseBad jet cleaning (only for HZyJetsOR_Extra downstream)
      {
        static const SG::ConstAccessor<char> acc_JetCleanLooseBad("DFCommonJets_jetClean_LooseBad");
        auto jetsCleaned = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
        for (const xAOD::Jet *jet : *jetsORExtraView) {
          if (acc_JetCleanLooseBad.isAvailable(*jet) && acc_JetCleanLooseBad(*jet))
            jetsCleaned->push_back(jet);
        }
        jetsORExtraView = std::move(jetsCleaned);
      }

      // Apply JVT cut to jetsORExtraView for downstream use
      {
        auto jetsWithJvt = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
        for (const xAOD::Jet *jet : *jetsORExtraView) {
          if (m_jvtSelection.isAvailable(*jet) && m_jvtSelection(*jet))
            jetsWithJvt->push_back(jet);
        }
        jetsORExtraView = std::move(jetsWithJvt);
      }

      ATH_CHECK(m_HZyElORPhotonsExtraHandle.record(std::move(elExtraView), sys, ctx));
      ATH_CHECK(m_HZyMuORPhotonsExtraHandle.record(std::move(muExtraView), sys, ctx));
      ATH_CHECK(m_HZyJetORExtraHandle.record(std::move(jetsORExtraView), sys, ctx));//jets overlaped with Z candidates, extra lepton and photon, with JVT
      ATH_CHECK(m_HZyJetORNoJvtHandle.record(std::move(jetsORNoJvtView), sys, ctx));//jets overlaped with Z candidates, extra lepton and photon, no JVT
      ATH_CHECK(m_HZyPhotonORHandle.record(std::move(photonsView), sys, ctx));
      ATH_CHECK(m_HZyJetORHandle.record(std::move(jetsView), sys, ctx));//jets overlaped with Z candidates and photon 
      ATH_CHECK(m_HZyElORPhotonsHandle.record(std::move(electronsORPhotonsView), sys, ctx));
      ATH_CHECK(m_HZyMuORPhotonsHandle.record(std::move(muonsORPhotonsView), sys, ctx));
    }

    return StatusCode::SUCCESS;
  }
} // namespace HZY
