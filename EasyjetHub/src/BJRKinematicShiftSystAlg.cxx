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
    return StatusCode::SUCCESS;
  }

  StatusCode BJRKinematicShiftSystAlg::execute(const EventContext& ctx)
  {
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      xAOD::JetContainer *jets = nullptr;
      ANA_CHECK(m_jetHandle.getCopy(jets, sys, ctx));

      // 0 unless sys is one of ours, then +-1
      const float mSign = sys.getParameterByBaseName(m_massSystName.value());
      const float pSign = sys.getParameterByBaseName(m_ptSystName.value());
      if (mSign == 0.f && pSign == 0.f)
        continue;

      const float mFactor =
          1.f + mSign * static_cast<float>(m_massShift.value());
      const float pFactor =
          1.f + pSign * static_cast<float>(m_ptShift.value());
      for (xAOD::Jet *jet : *jets)
      {
        m_bjrMassDec(*jet) = m_bjrMass(*jet) * mFactor;
        m_bjrPtDec(*jet) = m_bjrPt(*jet) * pFactor;
      }
    }
    return StatusCode::SUCCESS;
  }
}
