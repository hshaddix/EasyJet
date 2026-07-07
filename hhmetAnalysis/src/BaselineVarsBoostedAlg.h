/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HHMETANALYSIS_FINALVARSBOOSTEDALG
#define HHMETANALYSIS_FINALVARSBOOSTEDALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

namespace HHMET
{

  /// \brief An algorithm for counting containers
  class BaselineVarsBoostedAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    using AthHistogramAlgorithm::AthHistogramAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:
    // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
    // "someInfo"};

    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
      m_smallRjetHandle{ this, "smallRContainerInKey", "", "small R jets container to read" };
    CP::SysReadHandle<xAOD::JetContainer>
      m_largeRjetHandle{ this, "largeRContainerInKey", "", "Large-R jets container to read" };

    CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

    Gaudi::Property<std::string> m_bTagWP {this, "bTagWP", "", "B-tagging working point."};

    CP::SysReadDecorHandle<int> m_R10TruthLabel{"R10TruthLabel_R22v1", this};
    CP::SysReadDecorHandle<float> m_GN2Xv01_phbb = {"GN2Xv01_phbb", this};
    CP::SysReadDecorHandle<float> m_GN2Xv01_phcc = {"GN2Xv01_phcc", this};
    CP::SysReadDecorHandle<float> m_GN2Xv01_pqcd = {"GN2Xv01_pqcd", this};
    CP::SysReadDecorHandle<float> m_GN2Xv01_ptop = {"GN2Xv01_ptop", this};
    CP::SysReadDecorHandle<float> m_Tau2_wta = {"Tau2_wta", this};
    CP::SysReadDecorHandle<float> m_Tau3_wta = {"Tau3_wta", this};

    Gaudi::Property<bool> m_isMC
      { this, "isMC", false, "Is this simulation?" };

    Gaudi::Property<std::vector<std::string>> m_Fvars
      {this, "floatVariableList", {}, "Name list of floating variables"};

    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_decos;

    // clang-format off
    std::vector<std::string> m_vars{
      "boosted_h2_m_",
      "boosted_h2_pt_",
      "boosted_h2_eta_",
      "boosted_h2_phi_",
      "boosted_h2_E_",
      "boosted_hh_m_",
      "boosted_hh_pt_",
      "boosted_hh_deltaEta_",
      "boosted_hh_deltaPhi_",
    };

    std::vector<std::string> m_largeR_vars{
      "boosted_h1_m",
      "boosted_h1_pt",
      "boosted_h1_eta",
      "boosted_h1_phi",
      "boosted_h1_E",
      "boosted_h1_Tau32_wta",
      "boosted_h1_GN2Xv01_disc",
      "boosted_h1_GN2Xv01_phbb",
      "boosted_h1_GN2Xv01_pqcd",
      "boosted_h1_GN2Xv01_phcc",
      "boosted_h1_GN2Xv01_ptop",
      "boosted_h1_truthLabel"
    };
    // clang-format on

    float calculateGN2Xv01_disc(float phbb_score, float pqcd_score, float phcc_score, float ptop_score, float f_Hcc=0.02, float f_top=0.25) const {
      return std::log(phbb_score / (f_Hcc * phcc_score + f_top * ptop_score + (1.0f - f_top - f_Hcc) * pqcd_score));
    }

  };
}

#endif
