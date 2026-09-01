/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "BJRSystPropagationAlg.h"

namespace Easyjet
{
  BJRSystPropagationAlg::BJRSystPropagationAlg(const std::string &name,
                                           ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode BJRSystPropagationAlg::initialize()
  {
    ATH_CHECK(m_jetHandle.initialize(m_systematicsList));
    // The nominal handle is configured with a NOSYS container name (no
    // %SYS% token), so it resolves to the same container for every
    // systematic. It still has to be registered with the systematics
    // list so it can be retrieved inside the per-systematic loop.
    ATH_CHECK(m_nominalJetHandle.initialize(m_systematicsList));

    // The bJR outputs are plain SG::Decorators that overwrite the existing
    // bare bJR10v01_mass/_pt aux in place on each per-systematic jet; they are
    // not systematics-aware write handles, so there is nothing to initialize
    // against the systematics list here.

    // Initialise syst list (must come after all syst-aware inputs/outputs)
    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BJRSystPropagationAlg::execute(const EventContext& ctx)
  {
    // Retrieve the nominal (NOSYS) jet container once. The nominal handle
    // has no %SYS% token, so retrieving it with the empty systematic set
    // gives the single NOSYS container; it is the same for every loop
    // iteration below.
    const xAOD::JetContainer *nomJets = nullptr;
    ANA_CHECK(m_nominalJetHandle.retrieve(nomJets, CP::SystematicSet(), ctx));

    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // Overwrite the bare bJR10v01_mass/_pt aux IN PLACE on this systematic's
      // jet shallow copy. Each systematic has its own shallow copy, so writing
      // the (non-varying) bare aux name per copy yields per-systematic values.
      // The downstream SystVariableCopier reads this same bare aux off the jet
      // and snapshots it into Zcand0/1_bJR10v01_mass, so the histogram/Zcand
      // path picks up the variation with no copier change. For NOSYS the ratio
      // is 1 (ufo_sys == ufo_nom), so we write back the frozen nominal value --
      // a self-consistent reference. The nominal container is only ever read
      // (its NOSYS write is the identity), so reading bjr_nom off it stays the
      // frozen value regardless of systematic ordering.

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK(m_jetHandle.retrieve(jets, sys, ctx));

      // Per-systematic and nominal containers are shallow copies made at
      // the same point in the sequence (before any selection / reordering),
      // so they are index-aligned and the same size. Fail loudly if not.
      if (jets->size() != nomJets->size())
      {
        ATH_MSG_ERROR(
            "Per-systematic large-R jet container ("
            << jets->size() << ") and nominal container ("
            << nomJets->size() << ") differ in size; "
            << "cannot index-align for BJR propagation");
        return StatusCode::FAILURE;
      }

      for (size_t i = 0; i < jets->size(); ++i)
      {
        const xAOD::Jet *jet = (*jets)[i];
        const xAOD::Jet *nom = (*nomJets)[i];

        const float ufo_m_sys = jet->m();
        const float ufo_pt_sys = jet->pt();
        const float ufo_m_nom = nom->m();
        const float ufo_pt_nom = nom->pt();

        // Frozen nominal bJR decorations (identical on every shallow copy).
        const float bjr_m_nom = m_bjrMass(*nom);
        const float bjr_pt_nom = m_bjrPt(*nom);

        // Guard against a zero/negative nominal UFO four-vector (large-R
        // UFO mass can be ~0); fall back to the frozen nominal value so the
        // decoration is always written.
        const float out_m =
            (ufo_m_nom > 0.f) ? bjr_m_nom * (ufo_m_sys / ufo_m_nom)
                              : bjr_m_nom;
        const float out_pt =
            (ufo_pt_nom > 0.f) ? bjr_pt_nom * (ufo_pt_sys / ufo_pt_nom)
                               : bjr_pt_nom;

        m_bjrMassDec(*jet) = out_m;
        m_bjrPtDec(*jet) = out_pt;
      }
    }
    return StatusCode::SUCCESS;
  }
}
