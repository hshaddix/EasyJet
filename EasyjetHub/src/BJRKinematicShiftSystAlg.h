/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef EASYJETHUB_BJRKINEMATICSHIFTSYSTALG
#define EASYJETHUB_BJRKINEMATICSHIFTSYSTALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysCopyHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <PATInterfaces/SystematicSet.h>
#include <AthContainers/ConstAccessor.h>
#include <AthContainers/Decorator.h>
#include <xAODJet/JetContainer.h>

#include <string>
#include <vector>

namespace Easyjet
{
  // Customs are registered downstream of the jet sequence, so no
  // per-systematic container exists for them without this copy;
  // SysReadHandle would return NOSYS and corrupt the nominal jets.
  class BJRKinematicShiftSystAlg final : public AthHistogramAlgorithm
  {
  public:
    BJRKinematicShiftSystAlg(const std::string &name,
                             ISvcLocator *pSvcLocator);

    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) override;

  private:
    Gaudi::Property<double> m_massShift{
        this, "massShift", 0.02, "fractional BJR mass shift (up and down)"};

    Gaudi::Property<double> m_ptShift{
        this, "ptShift", 0.01, "fractional BJR pT shift (up and down)"};

    Gaudi::Property<std::string> m_massSystName{
        this, "massSystName", "XBB_BJR_MASS_SHIFT",
        "name of the BJR mass-shift systematic"};

    Gaudi::Property<std::string> m_ptSystName{
        this, "ptSystName", "XBB_BJR_PT_SHIFT",
        "name of the BJR pT-shift systematic"};

    CP::SysListHandle m_systematicsList{this};

    CP::SysCopyHandle<xAOD::JetContainer> m_jetHandle{
        this, "jets", "",
        "Large-R UFO jet container to copy per systematic"};

    SG::Decorator<float> m_bjrMassDec{"bJR10v01_mass"};
    SG::Decorator<float> m_bjrPtDec{"bJR10v01_pt"};
    SG::ConstAccessor<float> m_bjrMass{"bJR10v01_mass"};
    SG::ConstAccessor<float> m_bjrPt{"bJR10v01_pt"};

    // Indexed like m_systematicsList.systematicsVector(); non-custom
    // entries have apply == false.
    struct ShiftSpec
    {
      bool apply;
      float massFactor;
      float ptFactor;
    };
    std::vector<ShiftSpec> m_specs;
  };
}

#endif
