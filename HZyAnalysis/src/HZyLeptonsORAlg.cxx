/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "HZyLeptonsORAlg.h"
#include "HZyCommonUtils.h"

// C++ STL and ROOT includes
#include <cmath>
#include <algorithm>
#include <TVector2.h>

// Include necessary xAOD headers for const_cast and containers
#include <xAODEgamma/Electron.h>
#include <xAODMuon/Muon.h>

namespace HZY
{
  StatusCode HZyLeptonsORAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("         HZyLeptonsORAlg          \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_HZyElectronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_electronLIdLIsoDecorHandle.initialize(m_systematicsList, m_HZyElectronHandle));
    ATH_CHECK(m_muonMIdLIsoDecorHandle.initialize(m_systematicsList, m_HZyMuonHandle));

    ATH_CHECK(m_HZyElectronORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuonORHandle.initialize(m_systematicsList));

    // Initialise syst list
    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode HZyLeptonsORAlg::execute(const EventContext& ctx)
  {
    // Loop over all systematics
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name() != "") continue;

      const xAOD::ElectronContainer *HZyElectrons = nullptr;
      ANA_CHECK(m_HZyElectronHandle.retrieve(HZyElectrons, sys, ctx));

      const xAOD::MuonContainer *HZyMuons = nullptr;
      ANA_CHECK(m_HZyMuonHandle.retrieve(HZyMuons, sys, ctx));

      auto electrons_LIdLIso = std::make_unique<ConstDataVector<xAOD::ElectronContainer>>(SG::VIEW_ELEMENTS);
      auto muons_MIdLIso = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);

      // Copy pointers from retrieved containers into the view-containers
      for (const xAOD::Electron *el : *HZyElectrons)
      {
        if (m_electronLIdLIsoDecorHandle.get(*el, sys)){
          electrons_LIdLIso->push_back(el);
        }
      }

      // Custom Muon Splitting:
      // Combined, MuonStandAlone, SegmentTagged, CaloTagged, SiliconAssociatedForwardMuon
      auto calo_muons = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
      auto noncalo_muons = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
      for (const xAOD::Muon *mu : *HZyMuons)
      {
        if (m_muonMIdLIsoDecorHandle.get(*mu, sys)){
          muons_MIdLIso->push_back(mu);
          if (mu->muonType() == xAOD::Muon::CaloTagged) {
            calo_muons->push_back(mu);
          } else {
            noncalo_muons->push_back(mu);
          }
        }
      }

      // Overlap Removal Step 1: Staco Muon/Staco Muon OR
      auto noncalo_to_keep = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
      std::sort(noncalo_muons->begin(), noncalo_muons->end(),
                [](const auto &a, const auto &b){
                  const xAOD::Muon* ma = a;
                  const xAOD::Muon* mb = b;
                  return ma->pt() > mb->pt();
                  });

      // Fixed bug in Staco Muon OR: was previously comparing all muons to each other, 
      // now only compares each muon to the ones with higher pt (i.e. those that would be kept preferentially)
      // A: high pT; B: medium pT, overlaps with A; C: low pT, overlaps with B, but not A
      // Old code would keep A, remove B and removes C (since it only checks overlap with A)
      // New code keeps A, removes B and keep C (since it checks overlap with B as well)
      for (const auto& muProxy : *noncalo_muons)
      {
          const xAOD::Muon *mu = muProxy;
          if (!HZY::overlap(mu, *noncalo_to_keep, m_dR_StacoMu_StacoMu.value())) {
              noncalo_to_keep->push_back(mu);
          }
      }
      noncalo_muons->swap(*noncalo_to_keep);

      // Overlap Removal Step 2: Electron/Electron Same Track OR
      auto electrons_same_track_OR = std::make_unique<ConstDataVector<xAOD::ElectronContainer>>(SG::VIEW_ELEMENTS);
      auto et = [](const xAOD::Electron* e) {
        return (e && e->trackParticle() && e->caloCluster()) ? e->caloCluster()->e() / std::cosh(e->trackParticle()->eta()) : -1.0;
      };
      auto sameTrack = [](const xAOD::Electron* a, const xAOD::Electron* b) {
          const auto* ta = a ? a->trackParticle() : nullptr;
          const auto* tb = b ? b->trackParticle() : nullptr;
          if (!ta || !tb) return false;

          constexpr double eps = 1e-6;
          return std::abs(ta->d0() - tb->d0()) < eps &&
                std::abs(ta->z0() - tb->z0()) < eps &&
                std::abs(TVector2::Phi_mpi_pi(ta->phi() - tb->phi())) < eps &&
                std::abs(ta->qOverP() - tb->qOverP()) < eps;
      };

      std::sort(electrons_LIdLIso->begin(), electrons_LIdLIso->end(),
                [&](const auto& a, const auto& b) {
                    return et(a) > et(b);
                });

      for (const auto& elProxy : *electrons_LIdLIso) {
          const xAOD::Electron* el = elProxy;

          if (!el->trackParticle() || !el->caloCluster()) continue;
          bool duplicate = false;
          for (const auto& keptProxy : *electrons_same_track_OR) {
              const xAOD::Electron* kept = keptProxy;
              if (sameTrack(el, kept)) {
                  duplicate = true;
                  break;
              }
          }
          if (!duplicate) electrons_same_track_OR->push_back(el);
      }
      electrons_LIdLIso->swap(*electrons_same_track_OR);

      // Overlap Removal Step 3: Electron/Electron Calo Direction OR
      auto electrons_after_calo_OR = std::make_unique<ConstDataVector<xAOD::ElectronContainer>>(SG::VIEW_ELEMENTS);
      std::sort(electrons_LIdLIso->begin(), electrons_LIdLIso->end(),
                [](const auto& a, const auto& b){
                  const xAOD::Electron* ea = a;
                  const xAOD::Electron* eb = b;
                  return ea->pt() > eb->pt();
                });

      for (const auto& elProxy : *electrons_LIdLIso) {
          const xAOD::Electron* el = elProxy;
          bool overlap = false;
          for (const auto& keptProxy : *electrons_after_calo_OR) {
              const xAOD::Electron* kept = keptProxy;

              if (std::fabs(el->eta() - kept->eta()) < m_dEta_ElEl.value() &&
                  std::fabs(TVector2::Phi_mpi_pi(el->phi() - kept->phi())) < m_dPhi_ElEl.value()) {
                  overlap = true;
                  break;
              }
          }
          if (!overlap) {
              electrons_after_calo_OR->push_back(el);
          }
      }
      electrons_LIdLIso->swap(*electrons_after_calo_OR);

      // Overlap Removal Step 4: Electron/Staco Muon OR
      auto electrons_after_staco_muon_OR = std::make_unique<ConstDataVector<xAOD::ElectronContainer>>(SG::VIEW_ELEMENTS);
      const double drCut = m_dR_El_StacoMu.value();
      const double dr2Cut = drCut * drCut;

      for (const auto& elProxy : *electrons_LIdLIso) {
          const xAOD::Electron* el = elProxy;

          const auto* elTrk = el->trackParticle();
          if (!elTrk) continue;

          const double elEta = elTrk->eta();
          const double elPhi = elTrk->phi();

          bool overlap = false;

          for (const auto& muProxy : *noncalo_muons) {
              const xAOD::Muon* mu = muProxy;

              const auto* muTrk = mu->trackParticle(xAOD::Muon::InnerDetectorTrackParticle);

              const double muEta = muTrk ? muTrk->eta() : mu->eta();
              const double muPhi = muTrk ? muTrk->phi() : mu->phi();

              const double dEta = elEta - muEta;
              const double dPhi = TVector2::Phi_mpi_pi(elPhi - muPhi);

              if (dEta * dEta + dPhi * dPhi < dr2Cut) {
                  overlap = true;
                  break;
              }
          }

          if (!overlap) {
              electrons_after_staco_muon_OR->push_back(el);
          }
      }
      electrons_LIdLIso->swap(*electrons_after_staco_muon_OR);

      // Overlap Removal Step 5: Calo Muon/Staco Muon OR
      safeRemoveOverlap(*calo_muons, *noncalo_muons, m_dR_CaloMu_StacoMu.value());

      // Overlap Removal Step 6: Calo Muon/Electron OR
      safeRemoveOverlap(*calo_muons, *electrons_LIdLIso, m_dR_CaloMu_Electron.value());

      // Re-merge and Sort Muons
      muons_MIdLIso->clear();
      for (const auto& mu : *calo_muons)  muons_MIdLIso->push_back(mu);
      for (const auto& mu : *noncalo_muons) muons_MIdLIso->push_back(mu);

      std::sort(muons_MIdLIso->begin(), muons_MIdLIso->end(),
                [](const auto &a, const auto &b){
                  const xAOD::Muon* ma = a;
                  const xAOD::Muon* mb = b;
                  return ma->pt() > mb->pt();
                });

      ATH_CHECK(m_HZyElectronORHandle.record(std::move(electrons_LIdLIso), sys, ctx));
      ATH_CHECK(m_HZyMuonORHandle.record(std::move(muons_MIdLIso), sys, ctx));

    } // end loop over systematics

    return StatusCode::SUCCESS;
  }
} // namespace HZY
