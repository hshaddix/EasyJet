/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef EASYJETHUB_BJRSYSTPROPAGATIONALG
#define EASYJETHUB_BJRSYSTPROPAGATIONALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <AthContainers/ConstAccessor.h>
#include <AthContainers/Decorator.h>
#include <xAODJet/JetContainer.h>

namespace Easyjet
{
  /// Propagate large-R UFO jet systematic variations onto the bJR10v01
  /// regressed mass and pT, which the derivation produces nominal-only:
  ///
  ///   bjr_mass_sys = bjr_mass_nom * (ufo_mass_sys / ufo_mass_nom)
  ///   bjr_pt_sys   = bjr_pt_nom   * (ufo_pt_sys   / ufo_pt_nom)
  class BJRSystPropagationAlg final : public AthHistogramAlgorithm
  {
  public:
    BJRSystPropagationAlg(const std::string &name, ISvcLocator *pSvcLocator);

    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) override;

  private:
    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::JetContainer> m_jetHandle{
        this, "largeRJets", "",
        "Per-systematic large-R UFO jet container to read"};

    CP::SysReadHandle<xAOD::JetContainer> m_nominalJetHandle{
        this, "nominalLargeRJets", "",
        "Nominal (NOSYS) large-R UFO jet container to read"};

    SG::ConstAccessor<float> m_bjrMass{"bJR10v01_mass"};
    SG::ConstAccessor<float> m_bjrPt{"bJR10v01_pt"};

    SG::Decorator<float> m_bjrMassDec{"bJR10v01_mass"};
    SG::Decorator<float> m_bjrPtDec{"bJR10v01_pt"};
  };
}

#endif
