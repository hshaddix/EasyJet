/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HHH6BANALYSIS_FINALVARSSEMIBOOSTEDALG
#define HHH6BANALYSIS_FINALVARSSEMIBOOSTEDALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

namespace HHH6B
{

  /// \brief An algorithm for counting containers
  class BaselineVarsSemiBoostedAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    BaselineVarsSemiBoostedAlg(const std::string &name, ISvcLocator *pSvcLocator);

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

    CP::SysReadHandle<xAOD::JetContainer> m_LargeRJetHandle{ this, "jets", "boostedAnalysisLargeRJets", "Large-R jets container to read" };
    CP::SysReadHandle<xAOD::JetContainer> m_jetHandle{ this, "smallRjets", "resolvedAnalysisSmallRJets", "Jet container to read"};
      
    CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

    CP::SysReadDecorHandle<int> m_R10TruthLabel{"R10TruthLabel_R22v1", this};

    Gaudi::Property<bool> m_isMC
      { this, "isMC", false, "Is this simulation?" };

    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fdecos;

    std::vector<std::string> m_Fvars{
      // === Boosted variables ===
      "semiboosted_hhh_m",
      "semiboosted_hhh_pt",
      "semiboosted_h1h2_delta_eta",
      "semiboosted_h1h3_delta_eta",
      "semiboosted_h2h3_delta_eta",
      "semiboosted_h1h2_delta_phi",
      "semiboosted_h1h3_delta_phi",
      "semiboosted_h2h3_delta_phi",
      "semiboosted_h1_m",
      "semiboosted_h2_m",
      "semiboosted_h1_pt",
      "semiboosted_h2_pt",
      "semiboosted_h1_eta",
      "semiboosted_h2_eta",
      "semiboosted_h1_phi",
      "semiboosted_h2_phi",
      "semiboosted_h1_E",
      "semiboosted_h2_E",
      "semiboosted_h1_truthLabel",
      "semiboosted_h2_truthLabel",

      // === Resolved variables ===
      "semiboosted_h3_DeltaR56",
      "semiboosted_h3_DeltaEtaJet5Jet6",
      "semiboosted_h3_DeltaPhiJet5Jet6",
      "semiboosted_h3_Jet56Pt",
      "semiboosted_h3_Jet56Mass",
      "semiboosted_h1Jet5_deltaR",
      "semiboosted_h1Jet6_deltaR",
      "semiboosted_h2Jet5_deltaR",
      "semiboosted_h2Jet6_deltaR",
      "semiboosted_h3_Jet5_parentHiggsParentsMask",
      "semiboosted_h3_Jet6_parentHiggsParentsMask",

      // === Global variables ===
      "semiboosted_Thrust",
      "semiboosted_Sphericity",
      "semiboosted_TransverseSphericity",
      "semiboosted_Aplanarity",
      "semiboosted_Planarity",
      "semiboosted_h1h2_MassDiff",
      "semiboosted_h1h3_MassDiff",
      "semiboosted_h2h3_MassDiff"

    };

  };
}

#endif