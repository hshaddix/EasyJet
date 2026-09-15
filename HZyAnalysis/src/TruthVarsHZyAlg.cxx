/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "TruthVarsHZyAlg.h"

#include <AsgDataHandles/ReadHandle.h>
#include <TruthUtils/HepMCHelpers.h>

#include <cstdint>
#include <cstdlib>
#include <set>

namespace
{
  /// Self-copy chains and boson decay trees are short. This only stops a
  /// pathological truth record from sending the walk into an endless loop.
  constexpr int s_maxDepth = 100;

  /// Walk down a self-copy chain (Z -> Z -> ...) to the last copy, which is
  /// the one carrying the real decay products.
  const xAOD::TruthParticle *lastSelfCopy(const xAOD::TruthParticle *p,
                                          int depth = 0)
  {
    if (depth >= s_maxDepth) return p;
    for (std::size_t i = 0; i < p->nChildren(); i++)
    {
      const xAOD::TruthParticle *child = p->child(i);
      if (child && child->pdgId() == p->pdgId())
        return lastSelfCopy(child, depth + 1);
    }
    return p;
  }

  /// Collect the final copy of every Z boson below ptcl, so that the Z from
  /// H->Zy can be told apart from the associated one. Descending from the
  /// Higgs uses only child links, which are the ones the derivation keeps
  /// intact; walking up from a Z instead would depend on parent links that
  /// the thinning is free to drop.
  void collectZDescendants(const xAOD::TruthParticle *ptcl,
                           std::set<std::uint64_t> &uids,
                           int depth = 0)
  {
    if (depth >= s_maxDepth) return;
    for (std::size_t i = 0; i < ptcl->nChildren(); i++)
    {
      const xAOD::TruthParticle *child = ptcl->child(i);
      if (!child) continue;
      if (child->pdgId() == MC::Z0BOSON)
        uids.insert(lastSelfCopy(child)->uid());
      collectZDescendants(child, uids, depth + 1);
    }
  }

  /// Label a Z from the flavours of its direct decay products. The categories
  /// are tested in order of specificity instead of letting the last child win,
  /// so a Z -> ll that also radiated a photon is still leptonic rather than
  /// being dragged into the catch-all category by that photon.
  HZY::TruthVarsHZyAlg::AssocZDecay classifyZDecay(
      const xAOD::TruthParticle *z)
  {
    using AssocZDecay = HZY::TruthVarsHZyAlg::AssocZDecay;

    bool hasTau = false, hasLightLepton = false;
    bool hasNeutrino = false, hasQuarkOrGluon = false;

    for (std::size_t i = 0; i < z->nChildren(); i++)
    {
      const xAOD::TruthParticle *child = z->child(i);
      if (!child) continue;

      const int pdgId = child->pdgId();
      const int absPdgId = std::abs(pdgId);

      if (absPdgId == MC::TAU) hasTau = true;
      else if (absPdgId == MC::ELECTRON || absPdgId == MC::MUON)
        hasLightLepton = true;
      else if (MC::isSMNeutrino(pdgId)) hasNeutrino = true;
      else if (MC::isSMQuark(pdgId) || MC::isGluon(pdgId))
        hasQuarkOrGluon = true;
    }

    if (hasTau) return AssocZDecay::Tau;
    if (hasLightLepton) return AssocZDecay::Leptonic;
    if (hasNeutrino) return AssocZDecay::Invisible;
    if (hasQuarkOrGluon) return AssocZDecay::Hadronic;

    return AssocZDecay::None;
  }
}

namespace HZY
{
  StatusCode TruthVarsHZyAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("        TruthVarsHZyAlg          \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    // Only simulation has a truth record. On data the branch is still written,
    // with its default value, so that the two output formats stay identical.
    ATH_CHECK(m_truthBosonsKey.initialize(m_isMC));

    for (const std::string &var : m_intVariables)
    {
      CP::SysWriteDecorHandle<int> whandle{var + "_%SYS%", this};
      m_Ibranches.emplace(var, whandle);
      ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode TruthVarsHZyAlg::execute(const EventContext& ctx)
  {
    AssocZDecay label = AssocZDecay::None;

    if (m_isMC)
    {
      SG::ReadHandle<xAOD::TruthParticleContainer> truthBosons(
          m_truthBosonsKey, ctx);
      ATH_CHECK(truthBosons.isValid());
      label = classifyAssociatedZ(*truthBosons);
    }

    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      if (!m_doSystematics && sys.name() != "") continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      // Reset scalars
      for (const auto &var : m_intVariables)
        m_Ibranches.at(var).set(*event, -99, sys);

      // The truth record does not move with the reconstruction systematics,
      // so every variation gets the same label.
      m_Ibranches.at("AssocZ_DecayLabel").set(
          *event, static_cast<int>(label), sys);
    }

    return StatusCode::SUCCESS;
  }

  TruthVarsHZyAlg::AssocZDecay TruthVarsHZyAlg::classifyAssociatedZ(
      const xAOD::TruthParticleContainer &truthBosons) const
  {
    // Every Z that the Higgs decays to. These are the resonant H->Zy Z, and
    // are exactly what the associated Z is defined against.
    std::set<std::uint64_t> resonantZUIDs;
    for (const xAOD::TruthParticle *ptcl : truthBosons)
    {
      if (ptcl->pdgId() != MC::HIGGSBOSON) continue;
      collectZDescendants(lastSelfCopy(ptcl), resonantZUIDs);
    }

    AssocZDecay label = AssocZDecay::None;
    int nAssociated = 0;

    std::set<std::uint64_t> seenZUIDs;
    for (const xAOD::TruthParticle *ptcl : truthBosons)
    {
      if (ptcl->pdgId() != MC::Z0BOSON) continue;

      const xAOD::TruthParticle *finalZ = lastSelfCopy(ptcl);
      if (resonantZUIDs.contains(finalZ->uid())) continue;
      // The container holds every copy of the same Z, so only the first
      // sighting of a given final copy is classified.
      if (!seenZUIDs.insert(finalZ->uid()).second) continue;

      const AssocZDecay thisLabel = classifyZDecay(finalZ);
      if (thisLabel == AssocZDecay::None) continue;

      nAssociated++;
      if (nAssociated == 1) label = thisLabel;
    }

    if (nAssociated > 1)
      ATH_MSG_DEBUG("Found " << nAssociated << " associated Z bosons, "
                    << "labelling the first one.");

    return label;
  }
} // namespace HZY
