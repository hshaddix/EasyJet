/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ssWWTruthVarsAlg.h"

#include <AsgDataHandles/ReadDecorHandle.h>
#include <AsgDataHandles/ReadHandle.h>
#include <AsgDataHandles/WriteDecorHandle.h>
#include <AthContainers/AuxElement.h>

#include <TLorentzVector.h>
#include <xAODBase/IParticle.h>

#include <algorithm>
#include <cmath>

namespace
{
  constexpr float invalidFloat = -99.f;
  constexpr int invalidInt = -99;

  bool isChargedLepton(int pdgId)
  {
    const int absPdgId = std::abs(pdgId);
    return absPdgId == 11 || absPdgId == 13 || absPdgId == 15;
  }

  bool isLightChargedLepton(int pdgId)
  {
    const int absPdgId = std::abs(pdgId);
    return absPdgId == 11 || absPdgId == 13;
  }

  bool isNeutrino(int pdgId)
  {
    const int absPdgId = std::abs(pdgId);
    return absPdgId == 12 || absPdgId == 14 || absPdgId == 16;
  }

  int chargeFromLeptonPdgId(int pdgId)
  {
    return pdgId > 0 ? -1 : 1;
  }

  const xAOD::TruthParticle* findChild(
    const xAOD::TruthParticle* particle, bool (*predicate)(int))
  {
    if (!particle) return nullptr;
    for (size_t index = 0; index < particle->nChildren(); ++index) {
      const xAOD::TruthParticle* child = particle->child(index);
      if (child && predicate(child->pdgId())) return child;
    }
    return nullptr;
  }

  const xAOD::TruthParticle* findFinalParticleOfType(
    const xAOD::TruthParticle* particle, int pdgId)
  {
    if (!particle) return nullptr;
    for (size_t index = 0; index < particle->nChildren(); ++index) {
      const xAOD::TruthParticle* child = particle->child(index);
      if (child && child != particle && child->pdgId() == pdgId) {
        return findFinalParticleOfType(child, pdgId);
      }
    }
    return particle;
  }

  TLorentzVector dressedOrBareP4(const xAOD::TruthParticle* particle)
  {
    static const SG::ConstAccessor<float> ptDressed("pt_dressed");
    static const SG::ConstAccessor<float> etaDressed("eta_dressed");
    static const SG::ConstAccessor<float> phiDressed("phi_dressed");
    static const SG::ConstAccessor<float> eDressed("e_dressed");

    TLorentzVector p4;
    if (particle && ptDressed.isAvailable(*particle) &&
        etaDressed.isAvailable(*particle) &&
        phiDressed.isAvailable(*particle) &&
        eDressed.isAvailable(*particle)) {
      p4.SetPtEtaPhiE(ptDressed(*particle), etaDressed(*particle),
                      phiDressed(*particle), eDressed(*particle));
      return p4;
    }
    return particle ? particle->p4() : p4;
  }

  const xAOD::TruthParticle* matchTruthParticle(
    const xAOD::TruthParticle* target,
    const xAOD::TruthParticleContainer& candidates,
    float maxDeltaR)
  {
    if (!target) return nullptr;

    const TLorentzVector targetP4 = target->p4();
    const xAOD::TruthParticle* best = nullptr;
    float bestDeltaR = maxDeltaR;

    for (const xAOD::TruthParticle* candidate : candidates) {
      if (candidate->pdgId() != target->pdgId()) continue;
      const float deltaR = targetP4.DeltaR(dressedOrBareP4(candidate));
      if (deltaR < bestDeltaR) {
        bestDeltaR = deltaR;
        best = candidate;
      }
    }
    return best;
  }

  struct RecoLeptonInfo
  {
    const xAOD::IParticle* particle{nullptr};
    int pdgId{invalidInt};
    int index{invalidInt};
  };

  struct TruthLeptonInfo
  {
    const xAOD::TruthParticle* particle{nullptr};
    int pdgId{invalidInt};
    int parentWIndex{invalidInt};
    int recoLeptonIndex{invalidInt};
    int recoLeptonPdgId{invalidInt};
    float recoLeptonDeltaR{invalidFloat};
    TLorentzVector p4;
  };

  struct TruthNeutrinoInfo
  {
    const xAOD::TruthParticle* particle{nullptr};
    int pdgId{invalidInt};
    int parentWIndex{invalidInt};
    TLorentzVector p4;
  };

  struct TruthWInfo
  {
    const xAOD::TruthParticle* particle{nullptr};
    const xAOD::TruthParticle* chargedLepton{nullptr};
    const xAOD::TruthParticle* neutrino{nullptr};
    int truthLeptonIndex{invalidInt};
    int neutrinoIndex{invalidInt};
    int recoLeptonIndex{invalidInt};
    int recoLeptonPdgId{invalidInt};
    float recoLeptonDeltaR{invalidFloat};
  };
}

namespace ssWWVBS
{
  StatusCode ssWWTruthVarsAlg::initialize()
  {
    ATH_CHECK(m_eventInKey.initialize());
    ATH_CHECK(m_truthBosonInKey.initialize());
    ATH_CHECK(m_truthNeutrinoInKey.initialize());
    ATH_CHECK(m_truthElectronInKey.initialize());
    ATH_CHECK(m_truthMuonInKey.initialize());
    ATH_CHECK(m_recoElectronInKey.initialize());
    ATH_CHECK(m_recoMuonInKey.initialize());
    ATH_CHECK(m_recoElectronSelectedKey.initialize());
    ATH_CHECK(m_recoMuonSelectedKey.initialize());

    for (const std::string& var : m_floatVariables) {
      m_floatDecorIndex.emplace(var, m_floatDecorKeys.size());
      m_floatDecorKeys.emplace_back(m_eventInKey, var);
    }
    ATH_CHECK(m_floatDecorKeys.initialize());

    for (const std::string& var : m_intVariables) {
      m_intDecorIndex.emplace(var, m_intDecorKeys.size());
      m_intDecorKeys.emplace_back(m_eventInKey, var);
    }
    ATH_CHECK(m_intDecorKeys.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode ssWWTruthVarsAlg::execute(const EventContext& ctx)
  {
    SG::ReadHandle<xAOD::EventInfo> eventInfo(m_eventInKey, ctx);    ATH_CHECK(eventInfo.isValid());

    SG::ReadHandle<xAOD::TruthParticleContainer> truthBosons(m_truthBosonInKey, ctx);    ATH_CHECK(truthBosons.isValid());

    SG::ReadHandle<xAOD::TruthParticleContainer> truthNeutrinos(m_truthNeutrinoInKey, ctx);    ATH_CHECK(truthNeutrinos.isValid());

    SG::ReadHandle<xAOD::TruthParticleContainer> truthElectrons(m_truthElectronInKey, ctx);    ATH_CHECK(truthElectrons.isValid());

    SG::ReadHandle<xAOD::TruthParticleContainer> truthMuons(m_truthMuonInKey, ctx);    ATH_CHECK(truthMuons.isValid());

    SG::ReadHandle<xAOD::ElectronContainer> recoElectrons(m_recoElectronInKey, ctx);    ATH_CHECK(recoElectrons.isValid());

    SG::ReadHandle<xAOD::MuonContainer> recoMuons(m_recoMuonInKey, ctx);    ATH_CHECK(recoMuons.isValid());

    std::unordered_map<std::string, SG::WriteDecorHandle<xAOD::EventInfo, float>>
      fbranches;
    for (const auto& [var, index] : m_floatDecorIndex) {
      fbranches.emplace(var, m_floatDecorKeys[index]);
    }

    std::unordered_map<std::string, SG::WriteDecorHandle<xAOD::EventInfo, int>>
      ibranches;
    for (const auto& [var, index] : m_intDecorIndex) {
      ibranches.emplace(var, m_intDecorKeys[index]);
    }

    const xAOD::EventInfo& event = *eventInfo;
    for (const std::string& var : m_floatVariables) {
      fbranches.at(var)(event) = invalidFloat;
    }
    for (const std::string& var : m_intVariables) {
      ibranches.at(var)(event) = invalidInt;
    }

    auto writeP4 = [&fbranches, &event](const std::string& prefix,
                                        const TLorentzVector& p4) {
      fbranches.at(prefix + "_pt")(event) = p4.Pt();
      fbranches.at(prefix + "_eta")(event) = p4.Eta();
      fbranches.at(prefix + "_phi")(event) = p4.Phi();
      fbranches.at(prefix + "_E")(event) = p4.E();
    };

    std::vector<const xAOD::TruthParticle*> selectedTruthLeptons;
    selectedTruthLeptons.reserve(truthElectrons->size() + truthMuons->size());
    for (const xAOD::TruthParticle* electron : *truthElectrons) {
      selectedTruthLeptons.push_back(electron);
    }
    for (const xAOD::TruthParticle* muon : *truthMuons) {
      selectedTruthLeptons.push_back(muon);
    }

    std::vector<TruthWInfo> truthWs;
    for (const xAOD::TruthParticle* particle : *truthBosons) {
      if (std::abs(particle->pdgId()) != 24) continue;

      const xAOD::TruthParticle* finalW =
        findFinalParticleOfType(particle, particle->pdgId());
      if (finalW != particle) continue;

      const xAOD::TruthParticle* chargedLepton =
        findChild(finalW, isChargedLepton);
      const xAOD::TruthParticle* neutrino = findChild(finalW, isNeutrino);
      if (!chargedLepton || !neutrino) continue;

      truthWs.push_back({finalW, chargedLepton, neutrino});
    }

    std::sort(truthWs.begin(), truthWs.end(),
              [](const TruthWInfo& lhs, const TruthWInfo& rhs) {
                return lhs.particle->pt() > rhs.particle->pt();
              });

    std::vector<TruthLeptonInfo> truthLeptons;
    truthLeptons.reserve(truthWs.size());
    std::vector<TruthNeutrinoInfo> neutrinos;
    neutrinos.reserve(truthWs.size());

    for (size_t wIndex = 0; wIndex < truthWs.size(); ++wIndex) {
      const int parentWIndex = static_cast<int>(wIndex) + 1;

      const xAOD::TruthParticle* chargedLepton = truthWs[wIndex].chargedLepton;
      const xAOD::TruthParticle* dressedLepton = nullptr;
      if (isLightChargedLepton(chargedLepton->pdgId())) {
        for (const xAOD::TruthParticle* candidate : selectedTruthLeptons) {
          if (candidate->pdgId() != chargedLepton->pdgId()) continue;
          const float deltaR =
            chargedLepton->p4().DeltaR(dressedOrBareP4(candidate));
          if (deltaR < 0.05) {
            dressedLepton = candidate;
            break;
          }
        }
      }

      const xAOD::TruthParticle* leptonForOutput =
        dressedLepton ? dressedLepton : chargedLepton;
      truthLeptons.push_back({leptonForOutput, chargedLepton->pdgId(),
                              parentWIndex, invalidInt, invalidInt,
                              invalidFloat, dressedOrBareP4(leptonForOutput)});

      const xAOD::TruthParticle* neutrinoForOutput =
        matchTruthParticle(truthWs[wIndex].neutrino, *truthNeutrinos, 0.05);
      if (!neutrinoForOutput) neutrinoForOutput = truthWs[wIndex].neutrino;
      neutrinos.push_back({neutrinoForOutput, truthWs[wIndex].neutrino->pdgId(),
                           parentWIndex, neutrinoForOutput->p4()});
    }

    std::sort(truthLeptons.begin(), truthLeptons.end(),
              [](const TruthLeptonInfo& lhs, const TruthLeptonInfo& rhs) {
                return lhs.p4.Pt() > rhs.p4.Pt();
              });
    std::sort(neutrinos.begin(), neutrinos.end(),
              [](const TruthNeutrinoInfo& lhs, const TruthNeutrinoInfo& rhs) {
                return lhs.p4.Pt() > rhs.p4.Pt();
              });

    SG::ReadDecorHandle<xAOD::ElectronContainer, char> electronSelected(
      m_recoElectronSelectedKey, ctx);
    SG::ReadDecorHandle<xAOD::MuonContainer, char> muonSelected(
      m_recoMuonSelectedKey, ctx);

    std::vector<RecoLeptonInfo> recoLeptons;
    recoLeptons.reserve(recoElectrons->size() + recoMuons->size());

    for (const xAOD::Electron* electron : *recoElectrons) {
      if (!electronSelected(*electron)) continue;
      recoLeptons.push_back({electron,
                             static_cast<int>(-11 * electron->charge()),
                             invalidInt});
    }

    for (const xAOD::Muon* muon : *recoMuons) {
      if (!muonSelected(*muon)) continue;
      recoLeptons.push_back({muon, static_cast<int>(-13 * muon->charge()),
                             invalidInt});
    }

    std::sort(recoLeptons.begin(), recoLeptons.end(),
              [](const RecoLeptonInfo& lhs, const RecoLeptonInfo& rhs) {
                return lhs.particle->pt() > rhs.particle->pt();
              });
    for (size_t index = 0; index < recoLeptons.size(); ++index) {
      recoLeptons[index].index = static_cast<int>(index) + 1;
    }

    std::vector<bool> recoIsUsed(recoLeptons.size(), false);
    for (TruthLeptonInfo& truthLepton : truthLeptons) {
      if (!isLightChargedLepton(truthLepton.pdgId)) continue;

      float bestDeltaR = 0.1;
      int bestReco = invalidInt;
      for (size_t recoIndex = 0; recoIndex < recoLeptons.size(); ++recoIndex) {
        if (recoIsUsed[recoIndex]) continue;
        if (recoLeptons[recoIndex].pdgId != truthLepton.pdgId) continue;

        const float deltaR =
          truthLepton.p4.DeltaR(recoLeptons[recoIndex].particle->p4());
        if (deltaR < bestDeltaR) {
          bestDeltaR = deltaR;
          bestReco = static_cast<int>(recoIndex);
        }
      }

      if (bestReco < 0) continue;
      recoIsUsed[bestReco] = true;
      truthLepton.recoLeptonIndex = recoLeptons[bestReco].index;
      truthLepton.recoLeptonPdgId = recoLeptons[bestReco].pdgId;
      truthLepton.recoLeptonDeltaR = bestDeltaR;
    }

    for (size_t leptonIndex = 0; leptonIndex < truthLeptons.size();
         ++leptonIndex) {
      const int parentWIndex = truthLeptons[leptonIndex].parentWIndex;
      if (parentWIndex <= 0 ||
          parentWIndex > static_cast<int>(truthWs.size())) {
        continue;
      }

      TruthWInfo& truthW = truthWs[parentWIndex - 1];
      truthW.truthLeptonIndex = static_cast<int>(leptonIndex) + 1;
      truthW.recoLeptonIndex = truthLeptons[leptonIndex].recoLeptonIndex;
      truthW.recoLeptonPdgId = truthLeptons[leptonIndex].recoLeptonPdgId;
      truthW.recoLeptonDeltaR = truthLeptons[leptonIndex].recoLeptonDeltaR;
    }

    for (size_t neutrinoIndex = 0; neutrinoIndex < neutrinos.size();
         ++neutrinoIndex) {
      const int parentWIndex = neutrinos[neutrinoIndex].parentWIndex;
      if (parentWIndex <= 0 ||
          parentWIndex > static_cast<int>(truthWs.size())) {
        continue;
      }
      truthWs[parentWIndex - 1].neutrinoIndex =
        static_cast<int>(neutrinoIndex) + 1;
    }

    ibranches.at("nTruthWs")(event) = static_cast<int>(truthWs.size());
    ibranches.at("nTruthWLeptons")(event) =
      static_cast<int>(truthLeptons.size());
    ibranches.at("nTruthNeutrinos")(event) =
      static_cast<int>(neutrinos.size());

    const size_t maxObjects = 2;
    for (size_t index = 0; index < std::min(maxObjects, truthWs.size());
         ++index) {
      const std::string prefix = "TruthW_" + std::to_string(index + 1);
      writeP4(prefix, truthWs[index].particle->p4());
      fbranches.at(prefix + "_m")(event) = truthWs[index].particle->m();
      fbranches.at(prefix + "_recoLepton_dR")(event) =
        truthWs[index].recoLeptonDeltaR;

      ibranches.at(prefix + "_pdgId")(event) = truthWs[index].particle->pdgId();
      ibranches.at(prefix + "_status")(event) = truthWs[index].particle->status();
      ibranches.at(prefix + "_decayLeptonPdgId")(event) =
        truthWs[index].chargedLepton ? truthWs[index].chargedLepton->pdgId() :
                                       invalidInt;
      ibranches.at(prefix + "_decayNeutrinoPdgId")(event) =
        truthWs[index].neutrino ? truthWs[index].neutrino->pdgId() :
                                  invalidInt;
      ibranches.at(prefix + "_truthLeptonIndex")(event) =
        truthWs[index].truthLeptonIndex;
      ibranches.at(prefix + "_neutrinoIndex")(event) =
        truthWs[index].neutrinoIndex;
      ibranches.at(prefix + "_recoLeptonIndex")(event) =
        truthWs[index].recoLeptonIndex;
      ibranches.at(prefix + "_recoLeptonPdgId")(event) =
        truthWs[index].recoLeptonPdgId;
    }

    for (size_t index = 0; index < std::min(maxObjects, truthLeptons.size());
         ++index) {
      const std::string prefix = "TruthWLepton_" + std::to_string(index + 1);
      writeP4(prefix, truthLeptons[index].p4);
      fbranches.at(prefix + "_m")(event) = truthLeptons[index].p4.M();
      fbranches.at(prefix + "_recoLepton_dR")(event) =
        truthLeptons[index].recoLeptonDeltaR;

      ibranches.at(prefix + "_pdgId")(event) = truthLeptons[index].pdgId;
      ibranches.at(prefix + "_charge")(event) =
        chargeFromLeptonPdgId(truthLeptons[index].pdgId);
      ibranches.at(prefix + "_parentWIndex")(event) =
        truthLeptons[index].parentWIndex;
      ibranches.at(prefix + "_recoLeptonIndex")(event) =
        truthLeptons[index].recoLeptonIndex;
      ibranches.at(prefix + "_recoLeptonPdgId")(event) =
        truthLeptons[index].recoLeptonPdgId;
    }

    for (size_t index = 0; index < std::min(maxObjects, neutrinos.size());
         ++index) {
      const std::string prefix = "TruthNeutrino_" + std::to_string(index + 1);
      writeP4(prefix, neutrinos[index].p4);
      ibranches.at(prefix + "_pdgId")(event) = neutrinos[index].pdgId;
      ibranches.at(prefix + "_parentWIndex")(event) =
        neutrinos[index].parentWIndex;
    }

    return StatusCode::SUCCESS;
  }
}
