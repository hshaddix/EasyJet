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

    ATH_CHECK(m_jetHandle.initialize(m_systematicsList));

    CP::SystematicSet customs;
    for (int sign : {1, -1})
    {
      customs.insert(CP::SystematicVariation(m_massSystName.value(), sign));
      customs.insert(CP::SystematicVariation(m_ptSystName.value(), sign));
    }
    ATH_CHECK(m_systematicsList.addSystematics(customs, customs));
    ATH_CHECK(m_systematicsList.initialize());

    const std::string massUp =
        CP::SystematicVariation(m_massSystName, 1).name();
    const std::string massDn =
        CP::SystematicVariation(m_massSystName, -1).name();
    const std::string ptUp =
        CP::SystematicVariation(m_ptSystName, 1).name();
    const std::string ptDn =
        CP::SystematicVariation(m_ptSystName, -1).name();
    const float mUp = 1.f + static_cast<float>(m_massShift.value());
    const float mDn = 1.f - static_cast<float>(m_massShift.value());
    const float pUp = 1.f + static_cast<float>(m_ptShift.value());
    const float pDn = 1.f - static_cast<float>(m_ptShift.value());

    std::size_t nCustom = 0;
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      ShiftSpec spec{false, 1.f, 1.f};
      if (sys.name() == massUp)
        spec = {true, mUp, 1.f};
      else if (sys.name() == massDn)
        spec = {true, mDn, 1.f};
      else if (sys.name() == ptUp)
        spec = {true, 1.f, pUp};
      else if (sys.name() == ptDn)
        spec = {true, 1.f, pDn};
      if (spec.apply)
        ++nCustom;
      m_specs.push_back(spec);
    }

    ATH_MSG_INFO("registered " << nCustom
                 << " custom BJR kinematic-shift variations");
    return StatusCode::SUCCESS;
  }

  StatusCode BJRKinematicShiftSystAlg::execute(const EventContext& ctx)
  {
    const auto &sysVec = m_systematicsList.systematicsVector();
    for (std::size_t i = 0; i < sysVec.size(); ++i)
    {
      xAOD::JetContainer *jets = nullptr;
      ANA_CHECK(m_jetHandle.getCopy(jets, sysVec[i], ctx));

      const ShiftSpec &spec = m_specs[i];
      if (!spec.apply)
        continue;

      for (xAOD::Jet *jet : *jets)
      {
        m_bjrMassDec(*jet) = m_bjrMass(*jet) * spec.massFactor;
        m_bjrPtDec(*jet) = m_bjrPt(*jet) * spec.ptFactor;
      }
    }
    return StatusCode::SUCCESS;
  }
}
