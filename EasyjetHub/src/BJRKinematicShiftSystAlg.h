/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef EASYJETHUB_BJRKINEMATICSHIFTSYSTALG
#define EASYJETHUB_BJRKINEMATICSHIFTSYSTALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <PATInterfaces/SystematicSet.h>
#include <AthContainers/ConstAccessor.h>
#include <xAODEventInfo/EventInfo.h>

#include <optional>
#include <string>
#include <vector>

namespace Easyjet
{
  /// Register custom "template-morph" CP systematics that scale the
  /// bJR10v01 candidate-jet mass or pT by a fixed fraction.
  ///
  /// NOT a reconstruction re-run: for each custom systematic the event
  /// selection, working point and event weight all stay NOMINAL -- only the
  /// binned mass (or pT) is scaled. The XBB_BJR_MASS_SHIFT__1up slice is
  /// the NOSYS event set with NOSYS pT/wp/weight and the mass scaled by
  /// 1 + massShift at fill time.
  ///
  /// The shifted values reach the histograms because the SysWriteDecorHandles
  /// below register Zcand{0,1}_bJR10v01_{mass,pt} as AFFECTED by the custom
  /// systematics, so the ZbbHistogramAlg read handles resolve to our shifted
  /// decoration instead of falling back to NOSYS.
  ///
  /// SCHEDULING IS LOAD-BEARING: must run AFTER BaselineVarsZbbjCalibAlg (its
  /// SystVariableCopier writes the nominal Zcand decorations we read back, and
  /// our affecting-set registration must be the one the downstream read
  /// handles see) and BEFORE the ZbbHistogramAlg instances.
  ///
  class BJRKinematicShiftSystAlg final : public AthHistogramAlgorithm
  {
  public:
    BJRKinematicShiftSystAlg(const std::string &name,
                             ISvcLocator *pSvcLocator);

    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) override;

  private:
    // One mass NP and one pT NP -- the two independent morph directions.
    // A mass shift leaves pT nominal and vice versa; both candidate jets
    // shift coherently. NB the pT shift is prior-dominated in the fit
    // (sigma(alpha) ~0.8-1.0), so the quoted pT systematic scales linearly
    // with this fraction.
    Gaudi::Property<double> m_massShift{
        this, "massShift", 0.02, "fractional BJR mass shift (up and down)"};

    Gaudi::Property<double> m_ptShift{
        this, "ptShift", 0.01, "fractional BJR pT shift (up and down)"};

    // These appear verbatim on the H5 sys axis and in the ZCF runcards.
    Gaudi::Property<std::string> m_massSystName{
        this, "massSystName", "XBB_BJR_MASS_SHIFT",
        "name of the BJR mass-shift systematic"};

    Gaudi::Property<std::string> m_ptSystName{
        this, "ptSystName", "XBB_BJR_PT_SHIFT",
        "name of the BJR pT-shift systematic"};

    // Must match SystematicsSvc.nominalSystematicsName.
    Gaudi::Property<std::string> m_nominalSuffix{
        this, "nominalSuffix", "NOSYS",
        "suffix of the nominal EventInfo decoration to read"};

    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    // Base names MUST match what the SystVariableCopier writes, or the
    // histogram read handles will not find them.
    CP::SysWriteDecorHandle<float> m_zcand0Mass{
        "Zcand0_bJR10v01_mass_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_zcand0Pt{
        "Zcand0_bJR10v01_pt_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_zcand1Mass{
        "Zcand1_bJR10v01_mass_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_zcand1Pt{
        "Zcand1_bJR10v01_pt_%SYS%", this};

    // Frozen nominal Zcand decorations, named from m_nominalSuffix in
    // initialize().
    std::optional<SG::ConstAccessor<float>> m_zcand0MassNom;
    std::optional<SG::ConstAccessor<float>> m_zcand0PtNom;
    std::optional<SG::ConstAccessor<float>> m_zcand1MassNom;
    std::optional<SG::ConstAccessor<float>> m_zcand1PtNom;

    // The four custom variations (mass up/down, pT up/down), precomputed so
    // execute() is pure arithmetic. NOSYS and non-custom systematics are
    // absent: the copier already wrote those slices.
    struct ShiftSpec
    {
      CP::SystematicSet sys;
      float massFactor;
      float ptFactor;
    };
    std::vector<ShiftSpec> m_specs;
  };
}

#endif
