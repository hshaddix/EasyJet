/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "BJRKinematicShiftSystAlg.h"

#include <PATInterfaces/SystematicVariation.h>

namespace Easyjet
{
  BJRKinematicShiftSystAlg::BJRKinematicShiftSystAlg(
      const std::string &name, ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode BJRKinematicShiftSystAlg::initialize()
  {
    if (!(m_massShift > 0. && m_massShift < 1.) ||
        !(m_ptShift > 0. && m_ptShift < 1.))
    {
      ATH_MSG_ERROR("massShift and ptShift must be in (0, 1), got "
                    << m_massShift.value() << " and " << m_ptShift.value());
      return StatusCode::FAILURE;
    }

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_zcand0Mass.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_zcand0Pt.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_zcand1Mass.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_zcand1Pt.initialize(m_systematicsList, m_eventHandle));

    // Register our four variations (mass up/down, pT up/down) as both
    // recommended and affecting -- they are their own affecting set. Must
    // precede m_systematicsList.initialize().
    CP::SystematicSet customs;
    for (int sign : {1, -1})
    {
      customs.insert(CP::SystematicVariation(m_massSystName.value(), sign));
      customs.insert(CP::SystematicVariation(m_ptSystName.value(), sign));
    }
    ATH_CHECK(m_systematicsList.addSystematics(customs, customs));
    ATH_CHECK(m_systematicsList.initialize());

    const std::string sfx = "_" + m_nominalSuffix.value();
    m_zcand0MassNom.emplace("Zcand0_bJR10v01_mass" + sfx);
    m_zcand0PtNom.emplace("Zcand0_bJR10v01_pt" + sfx);
    m_zcand1MassNom.emplace("Zcand1_bJR10v01_mass" + sfx);
    m_zcand1PtNom.emplace("Zcand1_bJR10v01_pt" + sfx);

    // Keep only our four -- the copier owns every other slice. sign = +1 is
    // "__1up" (x[1+frac]), -1 is "__1down" (x[1-frac]).
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      for (int sign : {1, -1})
      {
        const std::string massVar =
            CP::SystematicVariation(m_massSystName, sign).name();
        const std::string ptVar =
            CP::SystematicVariation(m_ptSystName, sign).name();
        const float mf = 1.f + sign * static_cast<float>(m_massShift.value());
        const float pf = 1.f + sign * static_cast<float>(m_ptShift.value());

        if (sys.name() == massVar)
          m_specs.push_back({sys, mf, 1.f});
        else if (sys.name() == ptVar)
          m_specs.push_back({sys, 1.f, pf});
      }
    }

    ATH_MSG_INFO("registered " << m_specs.size()
                 << " custom BJR kinematic-shift variations");
    return StatusCode::SUCCESS;
  }

  StatusCode BJRKinematicShiftSystAlg::execute(const EventContext& ctx)
  {
    // EventInfo has no %SYS% token, so it is the same object for every
    // systematic; retrieve it once with the NOSYS set.
    const xAOD::EventInfo *event = nullptr;
    ANA_CHECK(m_eventHandle.retrieve(event, CP::SystematicSet(), ctx));

    const float m0 = (*m_zcand0MassNom)(*event);
    const float p0 = (*m_zcand0PtNom)(*event);
    const float m1 = (*m_zcand1MassNom)(*event);
    const float p1 = (*m_zcand1PtNom)(*event);

    for (const auto &spec : m_specs)
    {
      m_zcand0Mass.set(*event, m0 * spec.massFactor, spec.sys);
      m_zcand0Pt.set(*event, p0 * spec.ptFactor, spec.sys);
      m_zcand1Mass.set(*event, m1 * spec.massFactor, spec.sys);
      m_zcand1Pt.set(*event, p1 * spec.ptFactor, spec.sys);
    }
    return StatusCode::SUCCESS;
  }
}
