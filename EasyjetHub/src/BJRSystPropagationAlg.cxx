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
    ATH_CHECK(m_nominalJetHandle.initialize(m_systematicsList));

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
