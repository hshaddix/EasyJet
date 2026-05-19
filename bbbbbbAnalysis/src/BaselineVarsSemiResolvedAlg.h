/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HHH6BANALYSIS_FINALVARSSEMIRESOLVEDALG
#define HHH6BANALYSIS_FINALVARSSEMIRESOLVEDALG

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
  class BaselineVarsSemiResolvedAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    //BaselineVarsSemiResolvedAlg(const std::string &name, ISvcLocator *pSvcLocator);
    BaselineVarsSemiResolvedAlg(const std::string &name, ISvcLocator *pSvcLocator);

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute() override;
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
      // === Variables ===
      "semiresolved_hhh_m",
      "semiresolved_hhh_pt",
      "semiresolved_h1h2_delta_eta",
      "semiresolved_h1h3_delta_eta",
      "semiresolved_h2h3_delta_eta",
      "semiresolved_h1h2_delta_phi",
      "semiresolved_h1h3_delta_phi",
      "semiresolved_h2h3_delta_phi",
      "semiresolved_h1_m",
      "semiresolved_h1_pt",
      "semiresolved_h1_eta",
      "semiresolved_h1_phi",
      "semiresolved_h1_E",
      "semiresolved_h1_truthLabel",
      "semiresolved_h2_DeltaR34",
      "semiresolved_h2_DeltaEtaJet3Jet4",
      "semiresolved_h2_DeltaPhiJet3Jet4",
      "semiresolved_h2_Jet34Pt",
      "semiresolved_h2_Jet34Mass",
      "semiresolved_h3_DeltaR56",
      "semiresolved_h3_DeltaEtaJet5Jet6",
      "semiresolved_h3_DeltaPhiJet5Jet6",
      "semiresolved_h3_Jet56Pt",
      "semiresolved_h3_Jet56Mass",     
      "semiresolved_h1Jet3_deltaR",
      "semiresolved_h1Jet4_deltaR",
      "semiresolved_h1Jet5_deltaR",
      "semiresolved_h1Jet6_deltaR",
      "semiresolved_h2_Jet3_parentHiggsParentsMask",
      "semiresolved_h2_Jet4_parentHiggsParentsMask",
      "semiresolved_h3_Jet5_parentHiggsParentsMask",
      "semiresolved_h3_Jet6_parentHiggsParentsMask",
      "semiresolved_h1h2_MassDiff",
      "semiresolved_h1h3_MassDiff",
      "semiresolved_h2h3_MassDiff",
      "semiresolved_Thrust",
      "semiresolved_Sphericity",
      "semiresolved_TransverseSphericity",
      "semiresolved_Aplanarity",
      "semiresolved_Planarity",
    };
  };
}

#endif