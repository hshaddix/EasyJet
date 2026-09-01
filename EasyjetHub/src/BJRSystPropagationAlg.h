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
  /// Analytically propagate large-R UFO jet systematic variations
  /// (JES/JMS/JER) onto the bJR10v01 regressed mass and pT.
  ///
  /// The bJR10v01 mass/pT are produced by a GNN regression that runs
  /// nominal-only in the derivation, so they do not move when a CP
  /// systematic shifts the UFO jet four-vector. This algorithm applies a
  /// first-order, multiplicative propagation per systematic:
  ///
  ///   bjr_mass_sys = bjr_mass_nom * (ufo_mass_sys / ufo_mass_nom)
  ///   bjr_pt_sys   = bjr_pt_nom   * (ufo_pt_sys   / ufo_pt_nom)
  ///
  /// assuming the regression factor lambda = bjr/ufo is ~invariant under
  /// the variation. The nominal-calibrated UFO four-vector is read from the
  /// NOSYS container (the propagator must run after calibration and before
  /// any selection / pT-reordering so that the per-systematic and nominal
  /// containers stay index-aligned shallow copies).
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

    // Configured from python with readName(..., nominal=True); only ever
    // retrieved with the empty (NOSYS) systematic set.
    CP::SysReadHandle<xAOD::JetContainer> m_nominalJetHandle{
        this, "nominalLargeRJets", "",
        "Nominal (NOSYS) large-R UFO jet container to read"};

    // Frozen nominal bJR10v01 decorations: static aux baked into the
    // derivation, identical on every shallow copy, so read off the nominal jet.
    SG::ConstAccessor<float> m_bjrMass{"bJR10v01_mass"};
    SG::ConstAccessor<float> m_bjrPt{"bJR10v01_pt"};

    // OVERWRITE the bare bJR10v01 aux in place on each per-systematic jet
    // shallow copy. The bare names (no _%SYS% token) are deliberate: the
    // downstream SystVariableCopier reads the bare aux off the per-systematic
    // jet, so it snapshots the propagated value into Zcand0/1_bJR10v01_mass
    // per systematic with no copier change. Each per-systematic container is a
    // distinct shallow copy, so one name still yields per-systematic values.
    SG::Decorator<float> m_bjrMassDec{"bJR10v01_mass"};
    SG::Decorator<float> m_bjrPtDec{"bJR10v01_pt"};
  };
}

#endif
